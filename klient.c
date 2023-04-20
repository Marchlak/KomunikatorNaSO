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

#define USERNAME_LENGTH 25
#define COMMAND_LENGTH 10
#define FRAME_LENGTH 200
#define PATH_LENGTH 50

char username[(USERNAME_LENGTH + 1)];
char fifo_client_path[PATH_LENGTH];

void klient_zaloguj(char *user)
{
    printf("cos dziala xd");
    strcpy(username,user);
    char ramka_logowania[FRAME_LENGTH] = "";
    char readbuf[FRAME_LENGTH];
    int read_bytes;
    char komenda[(COMMAND_LENGTH + 1)];
    char wiadomosc[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];
    pid_t klient_pid = getpid();

    if (klient_tworzenie_pliku_fifo() != 0) {
        printf("Nie udalo sie utworzyc pliku FIFO klienta! Wylaczanie...\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie utworzyc pliku FIFO przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }
}

int klient_tworzenie_pliku_fifo() {
    strcat(fifo_client_path, "potoki/");
    strcat(fifo_client_path, username);
    strcat(fifo_client_path, ".fifo");
    //printf("Tworzenie potoku uzytkownika %s: %s\n", username, fifo_client_path);

    if (mkfifo(fifo_client_path, 0777) == -1) {
        if (errno != EEXIST) {
            printf("Nie udalo sie utworzyc pliku FIFO \"%s\" \n", fifo_client_path);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Nie udalo sie utworzyc potoku klienta %s: %s\n", username, fifo_client_path);
            closelog();
            return -1;
        } else {
            //printf("UWAGA: Plik FIFO \"%s\" juz istnieje\n", fifo_client_path);
            return 0;
        }
    } else {
        printf("Utworzono plik FIFO uzytkownika: \"%s\" \n", fifo_client_path);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Utworzono potok klienta %s: %s\n", username, fifo_client_path);
        closelog();
        return 0;
    }
}
