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
#include <stdio.h>
#include "frames.h"

#define USERNAME_LENGTH 25
#define COMMAND_LENGTH 10
#define FRAME_LENGTH 400
#define PATH_LENGTH 50
#define INT_DIGITS 19

char username[(USERNAME_LENGTH + 1)];
char fifo_client_path[PATH_LENGTH];
char download_path[PATH_LENGTH] = "";
int fd_serwer_fifo_WRITE;
char fifo_server_path[PATH_LENGTH];
int fd_serwer_fifo_WRITE;
int fd_fifo_klient_READ;

char *itoa(i) // funkcja itoa wzięta z netu
int i;
{
    static char buf[INT_DIGITS + 2];
    char *p = buf + INT_DIGITS + 1;
    if (i >= 0)
    {
        do
        {
            *--p = '0' + (i % 10);
            i /= 10;
        } while (i != 0);
        return p;
    }
    else
    {
        do
        {
            *--p = '0' - (i % 10);
            i /= 10;
        } while (i != 0);
        *--p = '-';
    }
    return p;
}

void extract_filename(char *path, char *filename)
{ // wyodrebnianie nazwy pliku z sciezki
    char *last_slash = strrchr(path, '/');

    if (last_slash == NULL)
    {
        strcpy(filename, path);
        return;
    }

    strcpy(filename, last_slash + 1);
}

int user_download(char *source, char *dest) // pobieranie
{

    int fd_src, fd_dst;
    struct stat stat_src;
    off_t offset = 0;
    char filename[PATH_LENGTH];
    char destination[PATH_LENGTH];
    strcpy(destination,dest);
    extract_filename(source, filename);
 
    if(destination[strlen(destination)-1]!='/')
    {
     strcat(destination, "/");
    }
    strcat(destination, filename);
    #ifdef DEBUG
    printf("%s %s \n", source, destination);
    #endif

    // Otwórz plik źródłowy w trybie do odczytu
    fd_src = open(source, O_RDONLY);
    if (fd_src == -1)
    {
        perror("Nie udało się otworzyć pliku źródłowego");
        return 0;
    }

    // Pobierz statystyki pliku źródłowego
    if (fstat(fd_src, &stat_src) == -1)
    {
        perror("Nie udało się pobrać informacji o pliku źródłowym");
        close(fd_src);
        return 0;
    }

    // Otwórz plik docelowy w trybie do zapisu
    fd_dst = open(destination, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_dst == -1)
    {
        perror("Nie udało się otworzyć pliku docelowego");
        close(fd_src);
        return 0;
    }

    // Skopiuj zawartość pliku źródłowego do pliku docelowego za pomocą funkcji sendfile
    if (sendfile(fd_dst, fd_src, &offset, stat_src.st_size) == -1)
    {
        perror("Nie udało się skopiować pliku");
        close(fd_src);
        close(fd_dst);
        return 0;
    }

    // Zamknij pliki
    close(fd_src);
    close(fd_dst);
    return 1;
    
}

void user_login(char *user, char *download_path_a, char *server_path) // logowanie klienta
{

    strcpy(fifo_server_path, server_path);
    strcpy(download_path_a, download_path);
    char login_frame[FRAME_LENGTH] = "";
    char readbuf[FRAME_LENGTH];
    int read_bytes;
    char command[(COMMAND_LENGTH + 1)];
    char message[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];

    pid_t user_pid = getpid();

    if (user_create_fifo_file() != 0)
    {
        printf("Nie udalo sie utworzyc pliku FIFO klienta! Wylaczanie...\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie utworzyc pliku FIFO przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

    if (fd_serwer_fifo_WRITE < 0)
    {
        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta - w celu zalogowania! Wylaczanie... \n \n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta \"%s\" - w celu zalogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    strcpy(login_frame, "/login ");
    strcat(login_frame, username);
    strcat(login_frame, " ");
    strcat(login_frame, itoa(user_pid));
    strcat(login_frame, " ");
    strcat(login_frame, download_path);
    login_frame[FRAME_LENGTH - 1] = '\0';
      #ifdef DEBUG 
     printf("Ramka logowania do wyslania: %s\n", login_frame);
     #endif

    if (user_send_frame(login_frame) != 0)
    {
        printf("Nie udalo sie wyslac ramki logowania do serwera! Wylaczanie...\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie wyslac ramki logowania do FIFO serwera - przez proces klienta \"%s\" - w celu zalogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    fd_fifo_klient_READ = open(fifo_client_path, O_RDONLY);

    if (fd_fifo_klient_READ < 0)
    {
        perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta - w celu odebrania komunikatu dot. logowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta \"%s\" - w celu odebrania komunikatu dot. logowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    read_bytes = read(fd_fifo_klient_READ, readbuf, sizeof(readbuf));

    if (read_bytes < 0)
    {
        perror("Nie udalo sie odczytac odpowiedzi serwera na ramke logowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie odczytac odpowiedzi serwera na ramke logowania klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (close(fd_serwer_fifo_WRITE) == -1)
    {
        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO serwera przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (close(fd_fifo_klient_READ) == -1)
    {
        perror("Nie udalo sie zamknac pliku FIFO klienta przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO klienta przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    readbuf[read_bytes] = '\0';

    if (frame_splitting_command(readbuf, command) == 0)
    {
        if (strcmp(command, "/alert") == 0)
        {
            if (frame_splitting_command_sender(readbuf, command, message) == 0)
            {
                #ifdef DEBUG
                 printf("pomyslnie podzielono ramke alert! - ramka: \"%s\" \n", readbuf);
                 #endif
                if ((strcmp(command, "") != 0) && (strcmp(message, "") != 0))
                {

                    if (strcmp(message, "LOGIN_SUCC") == 0)
                    {
                        printf("Zalogowano pomyslnie jako \"%s\"! \n", username);
                    }
                    else if (strcmp(message, "LOGIN_FAIL") == 0)
                    {
                        printf("Serwer odrzucil zadanie zalogowania jako \"%s\"! Wylaczanie...\n", username);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        printf("Nieprawidlowa tresc alertu: \"%s\"! Wylaczanie...\n", message);
                    }
                }
                else
                {
                    printf("Nieprawidlowa ramka typu alert bedaca odpowiedzia serwera na zadanie zalogowania! Wylaczanie...\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                printf("Blad podczas dzielenia ramki typu alert bedacej odpowiedzia serwera na zadanie zalogowania! Wylaczanie...\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("Nieprawidlowa odpowiedz serwera na zadanie zalogowania! Wylaczanie...\n");
            exit(EXIT_FAILURE);
        }
    }

    else
    {
        printf("Blad wyodrebniania komendy z ramki odpowiedzi serwera na zadanie zalogowania! Wylaczanie...\n");
        exit(EXIT_FAILURE);
    }
}

int user_create_fifo_file()
{
    strcat(fifo_client_path, "potoki/");
    strcat(fifo_client_path, username);
    strcat(fifo_client_path, ".fifo");
    #ifdef DEBUG
     printf("Tworzenie potoku uzytkownika %s: %s\n", username, fifo_client_path);
    #endif

    if (mkfifo(fifo_client_path, 0777) == -1)
    {
        if (errno != EEXIST)
        {
            printf("Nie udalo sie utworzyc pliku FIFO \"%s\" \n", fifo_client_path);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Nie udalo sie utworzyc potoku klienta %s: %s\n", username, fifo_client_path);
            closelog();
            return -1;
        }
        else
        {
            #ifdef DEBUG
             printf("UWAGA: Plik FIFO \"%s\" juz istnieje\n", fifo_client_path);
             #endif
            return 0;
        }
    }
    else
    {
        printf("Utworzono plik FIFO uzytkownika: \"%s\" \n", fifo_client_path);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Utworzono potok klienta %s: %s\n", username, fifo_client_path);
        closelog();
        return 0;
    }
}

int user_delete_fifo_file()
{
    if (remove(fifo_client_path) == 0)
    {
        #ifdef DEBUG
        printf("Plik FIFO uzytkownika %s zostal usuniety\n", username);
        #endif
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Usunieto potok klienta %s: %s\n", username, fifo_client_path);
        closelog();
        return 0;
    }
    else
    {
        printf("Nie udalo sie usunac pliku FIFO uzytkownika %s\n!", username);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie usunac potoku klienta %s: %s\n", username, fifo_client_path);
        closelog();
        return -1;
    }
}

int user_send_frame(char *msg)
{
    if (strlen(msg) >= FRAME_LENGTH)
    {
        printf("Ramka \"%s\" jest zbyt dluga by ja wyslac (%ld / %d)\n", msg, (strlen(msg) + 1), FRAME_LENGTH);
        return -1;
    }

    printf("Wysylanie ramki...\n");

    if (write(fd_serwer_fifo_WRITE, msg, strlen(msg)) == -1)
    {
        perror("Nie udalo sie wyslac ramki do serwera!");
        return -1;
    }
    else
    {
        printf("Wyslano ramke!\n");
        return 0;
    }
}

void user_read()
{
    char readbuf[FRAME_LENGTH];
    int read_bytes;
    char command[(COMMAND_LENGTH + 1)];
    char sender[(USERNAME_LENGTH + 1)];
    char recipent[(USERNAME_LENGTH + 1)];
    char message[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];

    printf("Otwarto plik FIFO klienta \"%s\" (tryb odczytu):  \n", fifo_client_path);

    while (1)
    {
        fd_fifo_klient_READ = open(fifo_client_path, O_RDONLY);

        if (fd_fifo_klient_READ < 0)
        {
            perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu przez proces klienta-dziecko!");
            exit(EXIT_FAILURE);
        }

        read_bytes = read(fd_fifo_klient_READ, readbuf, sizeof(readbuf));
        readbuf[read_bytes] = '\0';

        if (strlen(readbuf) > 0)
        {
            if (frame_splitting_command(readbuf, command) == 0)
            {
                if (strcmp(command, "/msg") == 0)
                {
                    if (frame_splitting_command_sender_receiver_message(readbuf, command, sender, recipent, message) == 0)
                    {
                        if ((strcmp(command, "") != 0) && (strcmp(sender, "") != 0) && (strcmp(recipent, "") != 0) && (strcmp(message, "") != 0))
                        {
                            if (close(fd_fifo_klient_READ) == -1)
                            {
                                perror("Nie udalo sie zamknac pliku FIFO klienta przez proces klienta-dziecko!");
                                exit(EXIT_FAILURE);
                            }
                            else
                            {
                                printf("%s: \"%s\" \n", sender, message); // wysylanie wiadomosci
                            }
                        }
                        else
                        {
                            printf("Nieprawidlowa ramka typu msg!\n");
                        }
                    }
                    else
                    {
                        printf("Blad podczas dzielenia ramki typu msg\n");
                    }
                }
                else if (strcmp(command, "/file") == 0)
                {
                    if (frame_splitting_command_sender_receiver_message(readbuf, command, sender, recipent, message) == 0)
                    {
                        if ((strcmp(command, "") != 0) && (strcmp(sender, "") != 0) && (strcmp(recipent, "") != 0) && (strcmp(message, "") != 0))
                        {
                            printf("Pobieranie pliku \"%s\" od uzytkownika %s?\n", message, sender);
                            if(user_download(message, download_path)==1)
                            {
                            setlogmask(LOG_UPTO(LOG_NOTICE));
                            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                            syslog(LOG_NOTICE, "Klient  \"%s\" pobral plik!\n", username);
                            closelog();
                            printf("Zakonczono pobieranie pliku!\n");
                            }
                            else
                            {
                            printf("Pobieranie zakonczone niepowodzeniem \n");
                            setlogmask(LOG_UPTO(LOG_NOTICE));
                            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                            syslog(LOG_NOTICE, "Klient  \"%s\" probowal pobrac plik lecz to sie nie powiodlo!\n", username);
                            closelog();
                            }
                        }
                        else
                        {
                            printf("Nieprawidlowa ramka typu file!\n");
                        }
                    }
                    else
                    {
                        printf("Blad podczas dzielenia ramki typu file\n");
                    }
                }
                else
                {
                    printf("Wykryto nieprawidlowa komende! (\"%s\") \n", command);
                }
            }
            else
            {
                printf("Wystapil blad wyodrebniania komendy z ramki\n");
            }
        }
    }
}

void user_sending()
{
    char command[(COMMAND_LENGTH + 1)];
    char recipent[(USERNAME_LENGTH + 1)];
    char message[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];
    char frame[FRAME_LENGTH];
    int stringlen;
    char readbuf[FRAME_LENGTH - 5];

    while (1)
    {
        fgets(readbuf, sizeof(readbuf), stdin);
        if (strlen(readbuf) > 1)
        {
            stringlen = strlen(readbuf);
            readbuf[stringlen - 1] = '\0';
            frame_splitting_command_receiver_message(readbuf, command, recipent, message);
            if ((strcmp(command, "") != 0) && (strcmp(recipent, "") != 0) && (strcmp(message, "") != 0))
            {
                if (strcmp(command, "/msg") == 0)
                {
                    strcpy(frame, command);
                    strcat(frame, " ");
                    strcat(frame, username);
                    strcat(frame, " ");
                    strcat(frame, recipent);
                    strcat(frame, " ");
                    strcat(frame, message);

                    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

                    if (fd_serwer_fifo_WRITE < 0)
                    {
                        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta-matki - w celu wyslania ramki typu msg!");
                        setlogmask(LOG_UPTO(LOG_NOTICE));
                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta-matki - w celu wyslania ramki typu msg przez uzytkownika: %s!", username);
                        closelog();
                    }

                    if (user_send_frame(frame) != 0)
                    {
                        printf("Nie udalo sie wyslac do serwera ramki typu msg!\n");
                        setlogmask(LOG_UPTO(LOG_NOTICE));
                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                        syslog(LOG_NOTICE, "Nie udalo sie wyslac do serwera ramki typu msg! przez uzytkownika: %s", username);
                        closelog();
                    }

                    if (close(fd_serwer_fifo_WRITE) == -1)
                    {
                        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta-matke!");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (strcmp(command, "/file") == 0)
                {
                    strcpy(frame, command);
                    strcat(frame, " ");
                    strcat(frame, username);
                    strcat(frame, " ");
                    strcat(frame, recipent);
                    strcat(frame, " ");
                    strcat(frame, message);

                    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

                    if (fd_serwer_fifo_WRITE < 0)
                    {
                        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta-matki - w celu ramki typu file!");
                    }

                    if (user_send_frame(frame) != 0)
                    {
                        printf("Nie udalo sie wyslac do serwera ramki typu file!\n");
                        setlogmask(LOG_UPTO(LOG_NOTICE));
                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                        syslog(LOG_NOTICE, "Nie udalo sie wyslac do serwera ramki typu file!\n");
                        closelog();
                    }

                    if (close(fd_serwer_fifo_WRITE) == -1)
                    {
                        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta-matke!");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (strcmp(command, "/users") == 0)
                {
                    strcpy(frame, command);
                    strcat(frame, " ");
                    strcat(frame, username);
                    strcat(frame, " ");
                    strcat(frame, recipent);
                    strcat(frame, " ");
                    strcat(frame, message);
                    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

                    if (fd_serwer_fifo_WRITE < 0)
                    {
                        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta-matki - w celu ramki typu users!");
                    }

                    if (user_send_frame(frame) != 0)
                    {
                        printf("Nie udalo sie wyslac do serwera ramki typu users!\n");
                        setlogmask(LOG_UPTO(LOG_NOTICE));
                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                        syslog(LOG_NOTICE, "Nie udalo sie wyslac do serwera ramki typu users!\n");
                        closelog();
                    }

                    if (close(fd_serwer_fifo_WRITE) == -1)
                    {
                        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta-matke!");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    printf("Nie ma takiej komendy!\n");
                }
            }
            else
            {
                printf("Nieprawidlowe polecenie!\n");
            }
        }
        else
        {
            printf("Za krotka wiadomosc!\n");
        }
    }
}

void user_logout()
{
    char login_frame[FRAME_LENGTH] = "";
    char readbuf[FRAME_LENGTH];
    int read_bytes;
    char command[(COMMAND_LENGTH + 1)];
    char message[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];

    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

    if (fd_serwer_fifo_WRITE < 0)
    {
        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta - w celu wylogowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO klienta w trybie zapisu - przez proces klienta \"%s\" - w celu wylogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    strcpy(login_frame, "/logout ");
    strcat(login_frame, username);
    login_frame[FRAME_LENGTH - 1] = '\0';
    #ifdef DEBUG
     printf("Ramka wylogowania do wyslania: %s\n", login_frame);
     #endif

    if (user_send_frame(login_frame) != 0)
    {
        printf("Nie udalo sie wyslac ramki wylogowania do serwera! Wylaczanie...\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie wyslac ramki wylogowania do serwera przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    fd_fifo_klient_READ = open(fifo_client_path, O_RDONLY);

    if (fd_fifo_klient_READ < 0)
    {
        perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta - w celu odebrania komunikatu dot. wylogowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta \"%s\" - w celu odebrania komunikatu dot. wylogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    read_bytes = read(fd_fifo_klient_READ, readbuf, sizeof(readbuf));

    if (read_bytes < 0)
    {
        perror("Nie udalo sie odczytac odpowiedzi serwera na ramke wylogowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie odczytac odpowiedzi serwera na ramke wylogowania - przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    sleep(1);

    if (close(fd_serwer_fifo_WRITE) == -1)
    {
        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO serwera przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (close(fd_fifo_klient_READ) == -1)
    {
        perror("Nie udalo sie zamknac pliku FIFO klienta przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO klienta przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (user_delete_fifo_file() == 0)
    {
        printf("Pomyslnie usunieto plik FIFO klienta.\n");
    }
    else
    {
        printf("Nie udalo sie usunac pliku FIFO klienta!\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie usunac pliku FIFO klienta \"%s\" !\n", username);
        closelog();
    }

    readbuf[read_bytes] = '\0';

    if (frame_splitting_command(readbuf, command) == 0)
    {
        if (strcmp(command, "/alert") == 0)
        {
            if (frame_splitting_command_sender(readbuf, command, message) == 0)
            {
                #ifdef DEBUG
                 printf("pomyslnie podzielono ramke alert! - ramka: \"%s\" \n", readbuf);
                #endif

                if ((strcmp(command, "") != 0) && (strcmp(message, "") != 0))
                {

                    if (strcmp(message, "LOGOUT_SUCC") == 0)
                    {
                        printf("Wylogowano pomyslnie! \n");
                    }
                    else if (strcmp(message, "LOGOUT_FAIL") == 0)
                    {
                        printf("Serwer odrzucil zadanie wylogowania!\n");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        printf("Nieprawidlowa tresc alertu: \"%s\"! Konczenie...\n", message);
                    }
                }
                else
                {
                    printf("Nieprawidlowa ramka typu alert bedaca odpowiedzia serwera na zadanie wylogowania! Konczenie...\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                printf("Blad podczas dzielenia ramki typu alert bedacej odpowiedzia serwera na zadanie wylogowania! Konczenie...\n");
                printf("ramka: \"%s\" \n", readbuf);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("Nieprawidlowa odpowiedz serwera na zadanie wylogowania! Konczenie...\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Blad wyodrebniania komendy z ramki odpowiedzi serwera na zadanie wylogowania! Konczenie...\n");
        exit(EXIT_FAILURE);
    }
}

void handler_SIGINT_user(int signum)
{
    write(1, "\nProgram zakonczony przez Ctrl+C!\n", 40);
    user_logout();
    exit(signum);
}

void handler_SIGQUIT_user(int signum)
{
    write(1, "\nProgram zakonczony przez Ctrl+\\!\n", 40);
    user_logout();
    exit(signum);
}

void handler_SIGUSR1_user(int signum)
{
    printf("Otrzymano informacje od serwera, iz jest on wylaczany - wylogowano wszystkich uzytkownika. Konczenie...\n");
    exit(EXIT_SUCCESS);
}

void handler_SIGCHLD_user_mother(int signum)
{
    write(1, "\nProces klienta-dziecka zostal zakonczony.\n", 44);
}
