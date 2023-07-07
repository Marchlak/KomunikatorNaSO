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
#include "user.h"
#include "frames.h"
#include <stdio.h>

#define FIFO_FOLDER "potoki/"
#define FIFO_SERVER_FILE "serwer.fifo"
#define LOCK_FILE "potoki/server.pid"
#define PATH_LENGTH 50
#define FRAME_LENGTH 400
#define NUMBER_OF_USERS 5
#define USERNAME_LENGTH 25
#define COMMAND_LENGTH 10

char fifo_server_path[PATH_LENGTH];
int fd_fifo_serwer_READ;
char readbuf[FRAME_LENGTH];
int read_bytes;
int ilosc_uzytkownikow = 0;
char t_users[NUMBER_OF_USERS][(USERNAME_LENGTH + 1)];
char download[NUMBER_OF_USERS][PATH_LENGTH];
int pids[NUMBER_OF_USERS];
char fifo_client_path[PATH_LENGTH];

void cleanup()
{
    // Usuwanie pliku blokady
    unlink(LOCK_FILE);
}
// przechwytywanie sygnału
void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        write(1, "\nProgram zakonczony przez Ctrl+C lub Ctrl+/!\n", 40);
        server_send_shudown();
        server_delete_files_after_shutdown();
        cleanup();
        exit(EXIT_SUCCESS);
    }
}

void server_send_shudown()
{
    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        if (pids[i] != 0)
            kill(pids[i], SIGUSR1);
    }
    printf("Wylaczanie serwera wszystkim klientom.\n");
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, "Serwer jest wylaczany a klienci wylogowywani (proces serwera odebral sygnal SIGINT lub SIGQUIT!\n");
    closelog();
}

int server_delete_files_after_shutdown()
{
    if (remove(fifo_server_path) == 0)
    {
        printf("Plik FIFO serwera usuniety pomyslnie!\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Usunieto potok serwera: %s\n", fifo_server_path);
        closelog();
    }
    else
    {
        printf("Wystapil blad podczas usuwania pliku FIFO serwera\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie usunac potoku serwera: %s\n", fifo_server_path);
        closelog();
    }

    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        if (strcmp(t_users[i], "") != 0)
        {
            char potok[PATH_LENGTH] = FIFO_FOLDER;
            strcat(potok, t_users[i]);
            strcat(potok, ".fifo");

            if (remove(potok) != 0)
            {
                setlogmask(LOG_UPTO(LOG_NOTICE));
                openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                syslog(LOG_NOTICE, "Nie udalo sie usunac (przez proces serwera) potoku klienta %s: %s\n", t_users[i], potok);
                closelog();
                return -1;
            }

            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Usunieto (przez proces serwera) potok klienta %s: %s\n", t_users[i], potok);
            closelog();
        }
    }

    return 0;
}

void server_print_users()
{
    printf("Zalogowani uzytkownicy:\n");

    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        printf("%d: %s : %d : %s\n", i, t_users[i], pids[i], download[i]);
    }
}

void server_send_users(char *sender, char *command)
{
    char temp_users[USERNAME_LENGTH * 5 + 10];
    strcpy(temp_users, "Dostepni uzytkownicy: ");
    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        if (strlen(t_users[i]) > 0)
        {
            strcat(temp_users, t_users[i]);
            strcat(temp_users, " ");
        }
    }
    #ifdef DEBUG
    printf("%s \n", temp_users);
    #endif
    server_send_message("/msg", "SERWER", sender, temp_users);
}

int serwer_check_the_user_is_in_the_array(char *user_nickname)
{
    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        if (strcmp(t_users[i], user_nickname) == 0)
        {
            return 0;
        }
    }
    return -1;
}

void server_read(char *path) // uruchamianie plus czytanie
{

    // lockowanie pliku
    strcpy(fifo_server_path, path);
    int lock_fd = open(LOCK_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (lock_fd == -1)
    {
        if (errno == EEXIST)
        {
            printf("Serwer już działa.\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            perror("Blad otwarcia");
            exit(EXIT_FAILURE);
        }
    }

    // tworzenie pliku fifo
    if (mkfifo(fifo_server_path, 0777) == -1)
    {

        if (errno != EEXIST)
        {
            printf("Nie udalo sie utworzyc pliku potoku FIFO \"%s\" \n", fifo_server_path);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Nie udalo sie utworzuc pliku potoku FIFO serwera!\n");
            closelog();
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Plik FIFO \"%s\" juz istnieje\n", fifo_server_path);
        }
    }
    else
    {
        printf("Utworzono plik potoku FIFO serwera: \"%s\" \n", fifo_server_path);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Utworzono pomyslnie plik potoku FIFO serwera!\n");
        closelog();
    }

    while (1)
    { // czytanie z pliku fifo
        printf("Otwieram potok FIFO i czekam na kolejne zapytania...\n");

        fd_fifo_serwer_READ = open(fifo_server_path, O_RDONLY);

        if (fd_fifo_serwer_READ < 0)
        {
            perror("Nie udalo sie otworzyc pliku potoku FIFO serwera w trybie odczytu przez proces serwera!");
            cleanup();
            server_delete_files_after_shutdown();
            exit(EXIT_FAILURE);
        }

        read_bytes = read(fd_fifo_serwer_READ, readbuf, sizeof(readbuf));
        readbuf[read_bytes] = '\0';
        // odbieranie z pliku fifo
        if ((int)strlen(readbuf) > 1)
        {
            printf("Odebrano ramke: \"%s\" o dlugosci %d bajtow.\n", readbuf, (int)strlen(readbuf));
            char command[(COMMAND_LENGTH + 1)];
            char sender[(USERNAME_LENGTH + 1)];
            char recipent[(USERNAME_LENGTH + 1)];
            char message[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];
            // odbieranie komend//
            if (frame_splitting_command(readbuf, command) == 0)
            {
                if (strcmp(command, "/login") == 0)
                {
                    if (frame_splitting_command_sender_receiver_message(readbuf, command, sender, recipent, message) == 0)
                    {
                        if ((strcmp(command, "") != 0) && (strcmp(sender, "") != 0) && (strcmp(recipent, "") != 0) && (strcmp(message, "") != 0))
                        {
                            if (close(fd_fifo_serwer_READ) == -1)
                            {
                                perror("Nie udalo sie zamknac pliku FIFO serwera, przed proba logowania nowego klienta przez proces serwera!");
                                break;
                            }
                            else
                            {
                                printf("Zamknieto plik FIFO serwera: \"%s\" \n", fifo_server_path);
                                if (server_user_login(sender, atoi(recipent), message) == 0)
                                {
                                }
                                server_print_users();
                                continue;
                            }
                        }
                        else
                        {
                            printf("Nie udalo sie przetworzyc wiadomosci!\n");
                        }
                    }
                    else
                    {
                        printf("blad podczas przetwarzania wiadomosci login\n");
                    }
                }
                else if (strcmp(command, "/msg") == 0)
                {
                    if (frame_splitting_command_sender_receiver_message(readbuf, command, sender, recipent, message) == 0)
                    {
                        if ((strcmp(command, "") != 0) && (strcmp(sender, "") != 0) && (strcmp(recipent, "") != 0) && (strcmp(message, "") != 0))
                        {
                            if (strlen(message) > 0)
                            {
                                if (serwer_check_the_user_is_in_the_array(recipent) == 0)
                                {
                                    if (server_send_message(command, sender, recipent, message) == 0)
                                        printf("Uzytkownik \"%s\" wyslal message do uzytkownika \"%s\". \n", sender, recipent);
                                    else
                                    {
                                        printf("Nie udalo sie przeslac wiadomosci od \"%s\" do \"%s\"! \n", sender, recipent);
                                        setlogmask(LOG_UPTO(LOG_NOTICE));
                                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                        syslog(LOG_NOTICE, "Nie udalo sie przeslac wiadomosci od \"%s\" do \"%s\"! \n", sender, recipent);
                                        closelog();
                                    }
                                }
                                else
                                {
                                    if (server_send_message(command, "SERWER", sender, "Nie ma takiego uzytkownika!") == 0)
                                    {
                                        printf("Uzytkownik \"%s\" probowal wyslac message plik do niezalogowanego uzytkownika \"%s\". \n", sender, recipent);
                                    }
                                    else
                                    {
                                        printf("Nie udalo sie poinformowac uzytkownika \"%s\" o braku mozliwosci czatowania z niezalogowanym uzytkownikiem \"%s\"! \n", sender, recipent);
                                        setlogmask(LOG_UPTO(LOG_NOTICE));
                                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                        syslog(LOG_NOTICE, "Nie udalo sie poinformowac uzytkownika \"%s\" o braku mozliwosci czatowania z niezalogowanym uzytkownikiem \"%s\"! \n", sender, recipent);
                                        closelog();
                                    }
                                }
                                printf("pomyslnie przetworzono message msg\n");
                            }
                            else
                            {
                                printf("Wiadomosc jest za krotka!\n");
                            }
                        }
                        else
                        {
                            printf("Nie udalo sie przetworzyc wiadomosci msg!\n");
                        }
                    }
                    else
                    {
                        printf("blad podczas przetwarzania wiadomosci msg\n");
                    }
                }
                else if (strcmp(command, "/logout") == 0)
                {
                    if (frame_splitting_command_sender(readbuf, command, sender) == 0)
                    {
                        #ifdef DEBUG
                            printf("pomyslnie podzielono message logout! frame: \"%s\" \n", readbuf);
                        #endif
          
                        if (close(fd_fifo_serwer_READ) == -1)
                        {
                            perror("Nie udalo sie zamknac pliku FIFO serwera, przed proba wylogowania nowego klienta przez proces serwera!");
                            break;
                        }
                        else
                        {
                            #ifdef DEBUG
                                printf("Zamknieto plik FIFO serwera (tryb odczytu): \"%s\" \n", fifo_server_path);
                            #endif

                            if (server_user_logout(sender) == 0)
                            {
                            }
                            server_print_users();
                            continue;
                        }
                    }
                    else
                    {
                        printf("Wystapil Blad podczas przetwarzania wiadomosci logout\n");
                    }
                }
                else if (strcmp(command, "/file") == 0)
                {
                    if (frame_splitting_command_sender_receiver_message(readbuf, command, sender, recipent, message) == 0)
                    {
                        if ((strcmp(command, "") != 0) && (strcmp(sender, "") != 0) && (strcmp(recipent, "") != 0) && (strcmp(message, "") != 0))
                        {
                             #ifdef DEBUG            
                             printf("pomyslnie przetworzono message file\n");
                             #endif
                            if (serwer_check_if_the_user_can_download(recipent) == 0)
                            {
                                if (server_send_message(command, sender, recipent, message) != 0)
                                {
                                    printf("Nie udalo sie przeslac zadania pobierania od \"%s\" do \"%s\"! \n", sender, recipent);
                                    setlogmask(LOG_UPTO(LOG_NOTICE));
                                    openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                    syslog(LOG_NOTICE, "Nie udalo sie przeslac zadania pobierania od \"%s\" do \"%s\"! \n", sender, recipent);
                                    closelog();
                                }
                                else
                                    printf("Uzytkownik \"%s\" przesyla plik dla uzytkownika \"%s\". \n", sender, recipent);
                            }
                            else
                            {
                                if (server_send_message("/msg", "SERWER", sender, "Ten uzytkownik nie moze pobierac plikow!") != 0)
                                {
                                    printf("Nie udalo sie przeslac informacji o braku mozliwosci pobierania przez klienta \"%s\" do \"%s\"! \n", recipent, sender);
                                    setlogmask(LOG_UPTO(LOG_NOTICE));
                                    openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                    syslog(LOG_NOTICE, "Nie udalo sie przeslac informacji o braku mozliwosci pobierania przez klienta \"%s\" do \"%s\"! \n", recipent, sender);
                                    closelog();
                                }
                            }
                        }
                        else
                        {
                            printf("Wystapil blad podczas dzielenia ramki file\n");
                        }
                    }
                    else
                    {
                        printf("Wystapil Blad podczas dzielenia ramki file\n");
                    }
                }
                else if (strcmp(command, "/users") == 0)
                {
                    if (frame_splitting_command_sender_receiver_message(readbuf, command, sender, recipent, message) == 0)
                    {
                    if ((strcmp(command, "") != 0) && (strcmp(sender, "") != 0) && (strcmp(recipent, "") != 0) && (strcmp(message, "") != 0))
                        {
                                    server_send_users(sender, command);
                                    printf("Udalo sie przeslac liste uzytkownikow do \"%s\"! \n", sender);
                                    setlogmask(LOG_UPTO(LOG_NOTICE));
                                    openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                    syslog(LOG_NOTICE, "Udalo sie przeslac liste uzytkownikow do \"%s\"! \n", sender);
                                    closelog();
                        }
                        else
                        {
                                    printf("Nie udalo sie przeslac listy uzytkownikow do \"%s\"! \n", sender);
                                    setlogmask(LOG_UPTO(LOG_NOTICE));
                                    openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                    syslog(LOG_NOTICE, "Nie udalo sie przeslac listy uzytkownikow do \"%s\"! \n", sender);
                                    closelog();
                        }
                    }
                    else
                    {
                        printf("blad podczas przetwarzania wiadomosci users\n");
                    }
                }

                else
                {
                    printf("blad wyodrebniania komendy z ramki\n");
                }
            }
        }
    }
}

int server_user_login(char *login, int user_pid, char *path_download)
{
    if (serwer_check_the_user_is_in_the_array(login) == 0)
    {
        // jest juz taki gosc zalogowany!
        printf("Wykryto probe podwojnego logowania uzytkownika \"%s\"! Odrzucono!\n", login);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Wykryto probe podwojnego logowania klienta %s!\n", login);
        closelog();

        if (server_send_alert(login, "LOGIN_FAIL") != 0)
        {
            printf("Nie udalo sie wyslac komunikatu o odrzuceniu zadania podwojnego logowania uzytkownika \"%s\"!\n",
                   login);
            return -1;
        }
        else
        {
            #ifdef DEBUG
                printf("Wyslano komunikat o odrzuceniu zadania podwojnego logowania uzytkownika \"%s\"!\n", login);
            #endif
            
            return -1;
        }
    }
    else
    {
        printf("Logowanie uzytkownika \"%s\"...\n", login);

        if (server_add_user_to_array(login, user_pid, path_download) != 0)
        {
            printf("Nie udalo sie zalogowac uzytkownika \"%s\"!\n", login);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Nie udalo sie zalogowac klienta %s!\n", login);
            closelog();

            if (server_send_alert(login, "LOGIN_FAIL") != 0)
            {
                printf("Nie udalo sie wyslac komunikatu o bledzie przy logowaniu uzytkownika \"%s\"!\n", login);
                return -1;
            }
            else
            {
                #ifdef DEBUG
                 printf("Wyslano komunikat o bledzie przy logowaniu uzytkownika \"%s\"!\n", login);
                 #endif
                return -1;
            }
        }
        else
        {
            printf("Pomyslnie zalogowano uzytkownika \"%s\"!\n", login);
#ifdef DEBUG
            printf("Logi zostaly zapisane![sudo tail -f /var/log/syslog]\n");
#endif
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Zalogowano pomyslnie klienta \"%s\" ! \n", login);
            closelog();
            if (server_send_alert(login, "LOGIN_SUCC") != 0)
            {
                printf("Nie udalo sie wyslac komunikatu o pomyslnym logowaniu uzytkownika \"%s\"!\n", login);
                return -1;
            }
            else
            {
                #ifdef DEBUG
                    printf("Wyslano komunikat o pomyslnym logowaniu uzytkownika \"%s\"!\n", login);
                #endif
                
                return 0;
            }
        }
    }
}

int server_add_user_to_array(char *user_nickname, int user_pid, char *path_download)
{
    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        if (strcmp(t_users[i], user_nickname) == 0)
        {
            printf("Jest juz taki uzytkownik w tablicy! Podwojne logowanie!\n");
            return -1;
        }
        else if (strcmp(t_users[i], "") == 0)
        {
            if (strcmp(strcpy(t_users[i], user_nickname), user_nickname) == 0)
            {
                if (strcmp(strcpy(download[i], path_download), path_download) == 0)
                {
                    pids[i] = user_pid;
                    return 0;
                }
                else
                {
                    printf("Nie udalo sie skopiowac sciezki pobierania logowanego uzytkownika do tablicy!\n");
                    return -1;
                }
            }
            else
            {
                printf("Nie udalo sie skopiowac nazwy logowanego uzytkownika do tablicy!\n");
                return -1;
            }
        }
    }
    printf("Nie udalo sie dodac uzytkownika do tablicy (brak miejsca w tablicy)\n");
    return -1;
}

int server_send_frame(int fd_fifo_klient_WRITE, char *frame)
{
    if (strlen(frame) >= FRAME_LENGTH)
    {
        printf("Ramka \"%s\" jest zbyt dluga by ja wyslac (%ld / %d)\n", frame, (strlen(frame) + 1), FRAME_LENGTH);
        return -1;
    }

    #ifdef DEBUG
        printf("Wysylanie ramki: \"%s\" \n", frame);
    #endif

    if (write(fd_fifo_klient_WRITE, frame, strlen(frame)) == -1)
    {
        perror("Nie udalo sie wyslac ramki przez serwer!");
        return -1;
    }
    else
    {
        #ifdef DEBUG
         printf("Wyslano ramke!\n");
         #endif
        return 0;
    }
}

int server_send_alert(char *recipent, char *message)
{
    int fd_fifo_klient_WRITE;
    char frame[FRAME_LENGTH];
    strcpy(fifo_client_path, "potoki/");
    strcat(fifo_client_path, recipent);
    strcat(fifo_client_path, ".fifo");
    #ifdef DEBUG
     printf("Szukanie potoku uzytkownika %s...\n", recipent);
    #endif

    if (access(fifo_client_path, F_OK) == 0)
    {
        printf("Znaleziono potok uzytkonika %s: %s\n", recipent, fifo_client_path);
    }
    else
    {
        printf("Nie znaleziono potoku uzytkonika %s: %s\n", recipent, fifo_client_path);
        return -1;
    }

    fd_fifo_klient_WRITE = open(fifo_client_path, O_WRONLY);

    if (fd_fifo_klient_WRITE < 0)
    {
        perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie zapisu przez proces serwera!");
        return -1;
    }

    strcpy(frame, "/alert ");
    strcat(frame, message);
    #ifdef DEBUG
     printf("Wysylanie ramki alertu: %s\n", frame);
     #endif

    if (server_send_frame(fd_fifo_klient_WRITE, frame) != 0)
    {
        printf("Nie udalo sie wyslac do klienta ramki z komunikatem!\n");
        return -1;
    }

    if (close(fd_fifo_klient_WRITE) == -1)
    {
        perror("Nie udalo sie zamknac pliku potoku FIFO klienta przez proces serwera!");
        return -1;
    }

    return 0;
}

int server_send_message(char *command, char *sender, char *recipent, char *message)
{
    int fd_fifo_klient_WRITE;
    char frame[FRAME_LENGTH];

    if (serwer_check_the_user_is_in_the_array(recipent) == 0)
    {
        strcpy(fifo_client_path, "potoki/");
        strcat(fifo_client_path, recipent);
        strcat(fifo_client_path, ".fifo");
        #ifdef DEBUG
        printf("Szukanie potoku uzytkownika %s...\n", recipent);
        #endif
        if (access(fifo_client_path, F_OK) == 0)
        {
            #ifdef DEBUG
             printf("Znaleziono potok uzytkownika %s: %s\n", recipent, fifo_client_path);
             #endif
        }
        else
        {
            printf("Nie znaleziono potoku uzytkonika %s!\n", recipent);
            return -1;
        }

        fd_fifo_klient_WRITE = open(fifo_client_path, O_WRONLY);

        if (fd_fifo_klient_WRITE < 0)
        {
            perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie zapisu przez proces serwera!");
            return -1;
        }

        strcpy(frame, command);
        strcat(frame, " ");
        strcat(frame, sender);
        strcat(frame, " ");
        strcat(frame, recipent);
        strcat(frame, " ");
        strcat(frame, message);

        if (server_send_frame(fd_fifo_klient_WRITE, frame) != 0)
        {
            printf("Nie udalo sie wyslac do klienta ramki z wiadomoscia!\n");
            return -1;
        }

        if (close(fd_fifo_klient_WRITE) == -1)
        {
            perror("Nie udalo sie zamknac pliku FIFO klienta przez serwer!");
            return -1;
        }

        return 0;
    }
    else
    {
        printf("Probowano wyslac message do niezalogowanego uzytkownika (od \"%s\" do \"%s\")!\n!", sender, recipent);
        return -1;
    }
}

int server_user_logout(char *login)
{
    if (serwer_check_the_user_is_in_the_array(login) == 0)
    {
        printf("Wylogowywanie uzytkownika \"%s\"...\n", login);

        if (server_delete_user_from_array(login) != 0)
        {
            printf("Nie udalo sie wylogowac uzytkownika \"%s\"!\n", login);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Nie udalo sie wylogowac klienta \"%s\" !\n", login);
            closelog();

            if (server_send_alert(login, "LOGOUT_FAIL") != 0)
            {
                printf("Nie udalo sie wyslac komunikatu o bledzie przy wylogowywaniu uzytkownika \"%s\"!\n", login);
                return -1;
            }
            else
            {
                #ifdef DEBUG
                 printf("Wyslano komunikat o bledzie przy wylogowywaniu uzytkownika \"%s\"!\n", login);
                 #endif
                return -1;
            }
        }
        else
        {
            printf("Pomyslnie wylogowano uzytkownika \"%s\"!\n", login);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Wylogowano pomyslnie klienta \"%s\" !\n", login);
            closelog();

            if (server_send_alert(login, "LOGOUT_SUCC") != 0)
            {
                printf("Nie udalo sie wyslac komunikatu o pomyslnym wylogowaniu uzytkownika \"%s\"!\n", login);
                return -1;
            }
            else
            {
                #ifdef DEBUG
                 printf("Logi zostaly zapisane![sudo tail -f /var/log/syslog]\n");
                 printf("Wyslano komunikat o pomyslnymserwer_usun_uzy wylogowaniu uzytkownika \"%s\"!\n", login);
                 #endif
                return 0;
            }
        }
    }
    else
    {
        printf("Wykryto probe wylogowania niezalogowanego uzytkownika - \"%s\"! Odrzucono!\n", login);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Wykryto probe wylogowania niezalogowanego uzytkownika (\"%s\") !\n", login);
        closelog();

        if (server_send_alert(login, "LOGOUT_FAIL") != 0)
        {
            printf("Nie udalo sie wyslac komunikatu o odrzuceniu zadania wylogowania niezalogowanego uzytkownika (\"%s\")!\n", login);
            return -1;
        }
        else
        {
            #ifdef DEBUG
             printf("Wyslano komunikat o odrzuceniu zadania wylogowania niezalogowanego uzytkownika (\"%s\")!\n", login);
             #endif
            return -1;
        }
    }
}

int server_delete_user_from_array(char *username)
{
    if (serwer_check_the_user_is_in_the_array(username) == 0)
    {
        for (int i = 0; i < NUMBER_OF_USERS; i++)
        {
            if (strcmp(t_users[i], username) == 0)
            {
                strcpy(t_users[i], "");
                strcpy(download[i], "");
                pids[i] = 0;
                return 0;
            }
        }
        printf("Blad dzialania funkcji server_delete_user_from_array! Nie znaleziono uzytkownika \"%s\" w tablicy t_users, wbrew odpowiedzi funkcji serwer_check_the_user_is_in_the_arra !\n",
               username);
        return -1;
    }
    else
    {
        printf("Nie mozna usunac uzytkownika, ktorego nie ma w tablicy uzytkownikow! (%s)\n", username);
        return -1;
    }
}

int serwer_check_if_the_user_can_download(char *recipent)
{
    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        if (strcmp(t_users[i], recipent) == 0)
        {
            if (strcmp(download[i], "BRAK") != 0 && strcmp(download[i], "") != 0)
            {
                return 0;
            }
            else
                return -1;
        }
    }
    return -1;
}
