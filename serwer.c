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
#include"klient.h"

#define FIFO_FOLDER "potoki/"
#define FIFO_SERVER_FILE "serwer.fifo"
#define LOCK_FILE "/var/run/server.pid"
#define PATH_LENGTH 50
#define FRAME_LENGTH 200
#define NUMBER_OF_USERS 5
#define USERNAME_LENGTH 25

char fifo_server_path[PATH_LENGTH];
int fd_fifo_serwer_READ;
char readbuf[FRAME_LENGTH];
int read_bytes;
int ilosc_uzytkownikow = 0;
char uzytkownicy[NUMBER_OF_USERS][(USERNAME_LENGTH + 1)];
char download[NUMBER_OF_USERS][PATH_LENGTH];
int pidy[NUMBER_OF_USERS];

void cleanup() {
    // Usuwanie pliku blokady
    unlink(LOCK_FILE);
}
//przechwytywani sygnału
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
          cleanup();
          exit(EXIT_SUCCESS);
    }
}

void serwer_start()
{
///sprawdzanie czy program działa
     struct flock lock;
    int lock_fd = open(LOCK_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (lock_fd == -1) {
        if (errno == EEXIST) {
            printf("Serwer już działa.\n");
            exit(EXIT_FAILURE);
        } else {
            perror("open error");
            exit(EXIT_FAILURE);
        }
        }
/////

//Tworzenie potoku fifo

 if ((strlen(FIFO_FOLDER) + strlen(FIFO_SERVER_FILE)) > PATH_LENGTH) {
        printf("Stala PATH_LENGTH jest za mala, by pomiescic sciezke do pliku FIFO serwera");
        exit(EXIT_FAILURE);
    } else {
        strcat(fifo_server_path, FIFO_FOLDER);
        strcat(fifo_server_path, FIFO_SERVER_FILE);
    }
//cd 
if (mkfifo(fifo_server_path, 0777) == -1) {

        if (errno != EEXIST) {
            printf("Nie udalo sie utworzyc pliku FIFO \"%s\" \n", fifo_server_path);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Nie udalo sie utworzuc pliku FIFO serwera!\n");
            closelog();
            exit(EXIT_FAILURE);
        } else {
            printf("Plik FIFO \"%s\" juz istnieje\n", fifo_server_path);
        }

    } else {
        printf("Utworzono plik FIFO serwera: \"%s\" \n", fifo_server_path);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Utworzono pomyslnie plik FIFO serwera!\n");
        closelog();
    }
 ///  
    while (1) {
        printf("Otwieram kolejke FIFO i czekam na kolejne zapytania...\n");
        
        fd_fifo_serwer_READ = open(fifo_server_path, O_RDONLY);

        if (fd_fifo_serwer_READ < 0) {
            perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie odczytu przez proces serwera!");
            cleanup();
            exit(EXIT_FAILURE);
        }

        read_bytes = read(fd_fifo_serwer_READ, readbuf, sizeof(readbuf));
        readbuf[read_bytes] = '\0';
        //odbieranie z pliku fifo
        if ((int) strlen(readbuf) > 1) {
        
        
        }

        }

}
