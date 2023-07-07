#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include "serwer.h"
#include "user.h"

#define DEBUGMAIN
#define PATH_LENGTH 50
#define USERNAME_LENGTH 25
#define FIFO_FOLDER "potoki/"
#define FIFO_SERVER_FILE "serwer.fifo"

char username[(USERNAME_LENGTH + 1)]; // nazwa uzytkownika
char fifo_server_path[PATH_LENGTH];   // sciezka serwera
volatile sig_atomic_t flaga = 1;
char download_path[PATH_LENGTH]; // sciezka pobierania

int main(int argc, char **argv)
{
    if ((strlen(FIFO_FOLDER) + strlen(FIFO_SERVER_FILE)) > PATH_LENGTH)
    { // ustawienie sciezek
        printf("Stala PATH_LENGTH jest za mala, by pomiescic sciezke do pliku FIFO serwera");
        exit(EXIT_FAILURE);
    }
    else
    {
        strcat(fifo_server_path, FIFO_FOLDER);
        strcat(fifo_server_path, FIFO_SERVER_FILE);
    }

    int option_cases; // wybrana opcja
    int digit_optind = 0;
    int this_option_optind = optind ? optind : 1;
    int option_index = 0; // indeks opcji
    int opt_d_count = 0;
    static struct option options_long[] = {// opcja do wybrania
                                           {"serwer", no_argument, 0, 's'},
                                           {"login", required_argument, 0, 'l'},
                                           {"download", required_argument, 0, 'd'},
                                           {0, 0, 0, 0}};

    while ((option_cases = getopt_long(argc, argv, "sd:l:", options_long, &option_index)) != -1)
    { // get opt
        switch (option_cases)
        {
        case 's':
            if (argc > 2)
            {
                printf("Za dużo opcji.. Kończenie programu \n");
                exit(0);
            }
            signal(SIGINT, signal_handler);
            signal(SIGTERM, signal_handler);
            server_read(fifo_server_path);
            cleanup();
            server_delete_files_after_shutdown();
            break;
        case 'd':
            opt_d_count++;
            if (opt_d_count > 1)
            {
                printf("Można wybrać tylko jedną opcje -d \n");
                exit(0);
            }
            if (strlen(optarg) > PATH_LENGTH)
            {
                printf("Stala PATH_LENGTH jest za mala, by pomiescic sciezke pobierania plikow dla klienta\n");
                exit(EXIT_FAILURE);
            }
            strcpy(download_path, optarg);
            if (mkdir(optarg, 0777) == -1)
            {
                if (errno == EEXIST)
                    printf("Nie ma potrzeby tworzyc folderu do pobierania, poniewaz juz takowy istnieje (%s).\n", download_path);
                else
                {
                    perror("Nie udalo sie utworzyc podanego folderu do pobierania!");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                printf("Folder do pobierania utworzony (%s).\n", download_path);
            }
            break;
        case 'l':
            if (strcmp(download_path, "") == 0)
            {
                strcpy(download_path, "BRAK");
            }

            if (access(fifo_server_path, F_OK) == 0)
            { // plik istnieje
                if (strlen(optarg) > USERNAME_LENGTH)
                {
                    printf("Nazwa uzytkownika \"%s\" jest zbyt dluga (maksymalna dozwolona dlugosc nazwy to  25 znakow) - zamykanie...",
                           optarg);
                    exit(EXIT_FAILURE);
                }
                else if (strlen(optarg) <= 1)
                {
                    printf("Nazwa uzytkownika \"%s\" jest zbyt krotka - zamykanie...\n", optarg);
                    exit(EXIT_FAILURE);
                }
                if (strcmp(strcpy(username, optarg), optarg) != 0)
                {
                    printf("Nie udalo sie skopiowac nazwy uzytkownika z argumentu optarg do tablicy username\n");
                    exit(EXIT_FAILURE);
                }

                user_login(username, download_path, fifo_server_path);

                pid_t pid = fork();
                int status;
                if (pid == -1)
                {
                    perror("Blad podczas forkowania procesu klienta");
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0)
                { // KLIENT-DZIECKO
                    user_read();
                    exit(EXIT_SUCCESS);
                }
                else
                { // KLIENT-RODZIC
                    signal(SIGCHLD, handler_SIGCHLD_user_mother);
                    signal(SIGINT, handler_SIGINT_user);
                    signal(SIGQUIT, handler_SIGINT_user);
                    signal(SIGUSR1, handler_SIGUSR1_user);

                    user_sending();

                    if (waitpid(pid, &status, 0) > 0)
                    {
                        if (WIFEXITED(status) && !WEXITSTATUS(status))
                            printf("program zakonczyl sie poprawnie\n");
                        else if (WIFEXITED(status) && WEXITSTATUS(status))
                        {
                            printf("program zakonczyl sie poprawnie, ale zwrocil wartosc niezerowa\n");
                        }
                        else
                            printf("program nie zakonczyl sie poprawnie\n");
                    }
                    else
                    {
                        printf("blad funkcji waitpid()\n");
                    }
                }
            }
            else
            {
                printf("Serwer jest wylaczony!\n");
                break;
            }
        default:
            printf("Brak takiej opcji");
            exit(EXIT_FAILURE);
        }
    }
    printf("Dostępne opcje: \n-s uruchomienie serwera \n-d 'sciezka pobierania' wlaczenie opcji odbierania plikow \n-l 'nick' zalogowanie użytkownika \nUwaga korzystając z opcji -d trzeba ją wpisać przed opcją l \n");
    return EXIT_SUCCESS;
}
