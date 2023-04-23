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
#include <sys/ipc.h>
#include <sys/msg.h>
#include"serwer.h"
#include"klient.h"

#define DEBUGMAIN
#define PATH_LENGTH 50
#define USERNAME_LENGTH 25
#define FIFO_FOLDER "potoki/"
#define FIFO_SERVER_FILE "serwer.fifo"


char username[(USERNAME_LENGTH + 1)];
char fifo_server_path[PATH_LENGTH];
volatile sig_atomic_t flaga = 1;
char download_path[PATH_LENGTH];




int main(int argc, char **argv)
{
   if ((strlen(FIFO_FOLDER) + strlen(FIFO_SERVER_FILE)) > PATH_LENGTH) {
        printf("Stala PATH_LENGTH jest za mala, by pomiescic sciezke do pliku FIFO serwera");
        exit(EXIT_FAILURE);
    } else {
        strcat(fifo_server_path, FIFO_FOLDER);
        strcat(fifo_server_path, FIFO_SERVER_FILE);
    }

    int option_cases; //wybrana opcja
    int digit_optind = 0;
    int this_option_optind = optind ? optind : 1;
    int option_index = 0; //indeks opcji
    int chosen_option = 0; //czy zostala wybrana opcja
    int opt_l_count = 0; // liczba podanych argumentów dla opcji -l
    int opt_d_count = 0; // liczba podanych argumentów dla opcji -d

    static struct option options_long[] = {
        {"serwer", no_argument, 0, 's'},
        {"login", required_argument, 0, 'l'},
        {"download", required_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    while ((option_cases = getopt_long(argc, argv, "sd:l:", options_long, &option_index)) != -1) { // dodac zeby mozna bylo wpisac jeden argument opcje niszczaca serwer
        switch (option_cases) {
            case 's':
            ///wybieranie tylko jednej opcji///
                if (chosen_option) { 
                    fprintf(stderr, "Error: Możesz wybrac tylko jedna opcje naraz\n");
                    return EXIT_FAILURE;
                }
                
                chosen_option = 's';
               ///wybieranie tylko jednej opcji///
               
               //przechwytywanie sygnału
               signal(SIGINT, signal_handler);
               signal(SIGTERM, signal_handler);
               
                serwer_start(fifo_server_path);
                break;
            case 'l':
            if (strcmp(download_path, "") == 0) 
            {
                    strcpy(download_path, "BRAK");
               }
               
                if (chosen_option) {
                    fprintf(stderr, "Error: Mozesz wybrac tylko jedna opcje naraz.\n");
                    return EXIT_FAILURE;
                }
                chosen_option = 'l';
                ///////////////////
                  
                if (access(fifo_server_path, F_OK) == 0) 
                { // file exists
                    if (strlen(optarg) > USERNAME_LENGTH) {
                        printf("Nazwa uzytkownika \"%s\" jest zbyt dluga (maksymalna dozwolona dlugosc nazwy to  25 znakow) - zamykanie...",
                               optarg);
                        exit(EXIT_FAILURE);
                    } else if (strlen(optarg) <= 1) {
                        printf("Nazwa uzytkownika \"%s\" jest zbyt krotka - zamykanie...\n", optarg);
                        exit(EXIT_FAILURE);
                    }              

                    if (strcmp(strcpy(username, optarg), optarg) != 0) {
                        printf("Nie udalo sie skopiowac nazwy uzytkownika z argumentu optarg do tablicy username\n");
                        exit(EXIT_FAILURE);
                    }
                    
                    printf("%s taka sciezka",fifo_server_path);
                    klient_zaloguj(username, fifo_server_path, download_path);
                    pid_t pid = fork();
                      int status;
                    if (pid == -1) {
                        perror("Blad podczas forkowania procesu klienta");
                        exit(EXIT_FAILURE);

                    } else if (pid == 0) { // KLIENT-DZIECKO
                        klient_czytanie();
                        exit(EXIT_SUCCESS);

                    } else { // KLIENT-RODZIC
                        signal(SIGCHLD, handler_SIGCHLD_klient_matka);
                        signal(SIGINT, handler_SIGINT_klient);
                        signal(SIGQUIT, handler_SIGINT_klient);
                        signal(SIGUSR1, handler_SIGUSR1_klient);

                        klient_nadawanie();

                        if (waitpid(pid, &status, 0) > 0) {
                            if (WIFEXITED(status) && !WEXITSTATUS(status))
                                printf("program execution successful\n");
                            else if (WIFEXITED(status) && WEXITSTATUS(status)) {
                                if (WEXITSTATUS(status) == 127) {
                                    // execv failed
                                    printf("execv failed\n");
                                } else
                                    printf("program terminated normally," " but returned a non-zero status\n");
                            } else
                                printf("program didn't terminate normally\n");
                        } else {
                            // waitpid() failed
                            printf("waitpid() failed\n");
                        }
                    }
                    }
                    else {
                    printf("Serwer jest wylaczony!\n");
                    break;
                }
                    
                
                
                break;
            case 'd':
                if (chosen_option) {
                    fprintf(stderr, "Error: Mozesz wybrac tylko jedna opcje naraz.\n");
                    return EXIT_FAILURE;
                }
                if(opt_l_count > 0)
                {
                fprintf(stderr, "Opcja -l może przymować tylko jeden argument");
                exit(EXIT_FAILURE);
                }
                chosen_option = 'd';
                printf("download \n");
                if (optarg) {
                    printf("Zalogowano uzytkownika %s\n", optarg);
                }
                break;
            case '?':
            printf("bład przy wyborze opcji zakończenie programu");
            //dopisac zeby cos do logow wrzucalo
                break;
        }
    }
    //komentarz
    return EXIT_SUCCESS;
}

