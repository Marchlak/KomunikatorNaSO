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
#define COMMAND_LENGTH 10

char fifo_server_path[PATH_LENGTH];
int fd_fifo_serwer_READ;
char readbuf[FRAME_LENGTH];
int read_bytes;
int ilosc_uzytkownikow = 0;
char uzytkownicy[NUMBER_OF_USERS][(USERNAME_LENGTH + 1)];
char download[NUMBER_OF_USERS][PATH_LENGTH];
int pidy[NUMBER_OF_USERS];
char fifo_client_path[PATH_LENGTH];

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

void handler_SIGINT_serwer(int signum) {
    write(1, "\nProgram zakonczony przez Ctrl+C!\n", 40);
    serwer_rozeslij_shutdown();
   serwer_usun_pliki_po_wylaczeniu_serwera();
    exit(signum);
}

void serwer_rozeslij_shutdown() {
    for (int i = 0; i < NUMBER_OF_USERS; i++) {
        if (pidy[i] != 0)
            kill(pidy[i], SIGUSR1);
    }
    printf("Zasygnalizowano wylaczanie serwera wszystkim klientom.\n");
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, "Serwer jest wylaczany a klienci wylogowywani (proces serwera odebral sygnal SIGINT lub SIGQUIT!\n");
    closelog();
}

int serwer_usun_pliki_po_wylaczeniu_serwera() {
    if (remove(fifo_server_path) == 0) {
        printf("Plik FIFO serwera usuniety pomyslnie!\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Usunieto potok serwera: %s\n", fifo_server_path);
        closelog();
    } else {
        printf("Wystapil blad podczas usuwania pliku FIFO serwera\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie usunac potoku serwera: %s\n", fifo_server_path);
        closelog();
    }

    for (int i = 0; i < NUMBER_OF_USERS; i++) {
        if (strcmp(uzytkownicy[i], "") != 0) {
            char potok[PATH_LENGTH] = FIFO_FOLDER;
            strcat(potok, uzytkownicy[i]);
            strcat(potok, ".fifo");

            if (remove(potok) != 0) {
                setlogmask(LOG_UPTO(LOG_NOTICE));
                openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                syslog(LOG_NOTICE, "Nie udalo sie usunac (przez proces serwera) potoku klienta %s: %s\n", uzytkownicy[i], potok);
                closelog();
                return -1;
            }

            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Usunieto (przez proces serwera) potok klienta %s: %s\n", uzytkownicy[i], potok);
            closelog();
        }
    }

    return 0;
} 

void serwer_wypisz_tablice_uzytkownikow() 
{
    printf("Zalogowani uzytkownicy:\n");

    for (int i = 0; i < NUMBER_OF_USERS; i++) 
    {
        printf("%d: %s : %d : %s\n", i, uzytkownicy[i], pidy[i], download[i]);
    }
}

int serwer_sprawdz_czy_uzytkownik_jest_w_tablicy_uzytkownikow(char *nazwa_uzytkownika) 
{
    for (int i = 0; i < NUMBER_OF_USERS; i++) {
        if (strcmp(uzytkownicy[i], nazwa_uzytkownika) == 0) {
            return 0;
        }
    }
    return -1;
}

void serwer_start(char *path)
{
strcpy (fifo_server_path,path);
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
            printf("Odebrano ramke: \"%s\" o dlugosci %d bajtow.\n", readbuf, (int) strlen(readbuf));
            char komenda[(COMMAND_LENGTH + 1)];
            char nadawca[(USERNAME_LENGTH + 1)];
            char odbiorca[(USERNAME_LENGTH + 1)];
            char wiadomosc[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];
            
            if (podziel_ramke_1(readbuf, komenda) == 0) 
            {
            if (strcmp(komenda, "/login") == 0) {
                    if (podziel_ramke_4(readbuf, komenda, nadawca, odbiorca, wiadomosc) == 0) {
                    printf("AAAAAAAAAAAAAAAAAA \n");
                        if ((strcmp(komenda, "") != 0) && (strcmp(nadawca, "") != 0) && (strcmp(odbiorca, "") != 0) && (strcmp(wiadomosc, "") != 0)) 
                        {
                            if (close(fd_fifo_serwer_READ) == -1) 
                            {
                                perror("Nie udalo sie zamknac pliku FIFO serwera, przed proba logowania nowego klienta przez proces serwera!");
                                break;
                            } else 
                            {
                                printf("Zamknieto plik FIFO serwera (tryb odczytu): \"%s\" \n", fifo_server_path);
                                if (serwer_zaloguj(nadawca, atoi(odbiorca), wiadomosc) == 0) {}
                                serwer_wypisz_tablice_uzytkownikow();
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
                else if (strcmp(komenda, "/msg") == 0) 
                {
                    if (podziel_ramke_4(readbuf, komenda, nadawca, odbiorca, wiadomosc) == 0) {
                        if ((strcmp(komenda, "") != 0) && (strcmp(nadawca, "") != 0) && (strcmp(odbiorca, "") != 0) && (strcmp(wiadomosc, "") != 0)) {
                            if (strlen(wiadomosc) > 0) {
                                if (serwer_sprawdz_czy_uzytkownik_jest_w_tablicy_uzytkownikow(odbiorca) == 0) {
                                    if(serwer_wyslij_wiadomosc(komenda, nadawca, odbiorca, wiadomosc) == 0)
                                        printf("Uzytkownik \"%s\" wyslal wiadomosc do uzytkownika \"%s\". \n", nadawca, odbiorca);
                                    else {
                                        printf("Nie udalo sie przeslac wiadomosci od \"%s\" do \"%s\"! \n", nadawca, odbiorca);
                                        setlogmask(LOG_UPTO(LOG_NOTICE));
                                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                        syslog(LOG_NOTICE, "Nie udalo sie przeslac wiadomosci od \"%s\" do \"%s\"! \n", nadawca, odbiorca);
                                        closelog();
                                    }
                                } else {
                                    if( serwer_wyslij_wiadomosc(komenda, "SERWER", nadawca, "Nie ma takiego uzytkownika!") == 0 ){
                                        printf("Uzytkownik \"%s\" probowal wyslac wiadomosc plik do niezalogowanego uzytkownika \"%s\". \n", nadawca, odbiorca);
                                    }else{
                                        printf("Nie udalo sie poinformowac uzytkownika \"%s\" o braku mozliwosci czatowania z niezalogowanym uzytkownikiem \"%s\"! \n", nadawca, odbiorca);
                                        setlogmask(LOG_UPTO(LOG_NOTICE));
                                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                                        syslog(LOG_NOTICE, "Nie udalo sie poinformowac uzytkownika \"%s\" o braku mozliwosci czatowania z niezalogowanym uzytkownikiem \"%s\"! \n", nadawca, odbiorca);
                                        closelog();
                                    }
                                }
                                printf("pomyslnie przetworzono wiadomosc msg\n");
                            } else {
                                printf("Wiadomosc jest za krotka!\n");
                            }
                        } else {
                            printf("Nie udalo sie przetworzyc wiadomosci msg!\n");
                        }
                    } else {
                        printf("blad podczas przetwarzania wiadomosci msg\n");
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

int serwer_zaloguj(char *login, int klient_pid, char *sciezka_pobierania) {
    if (serwer_sprawdz_czy_uzytkownik_jest_w_tablicy_uzytkownikow(login) == 0) {
        //jest juz taki gosc zalogowany!
        printf("Wykryto probe podwojnego logowania uzytkownika \"%s\"! Odrzucono!\n", login);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Wykryto probe podwojnego logowania klienta %s!\n", login);
        closelog();

        if (serwer_wyslij_komunikat(login, "LOGIN_FAIL") != 0) {
            printf("Nie udalo sie wyslac komunikatu o odrzuceniu zadania podwojnego logowania uzytkownika \"%s\"!\n",
                   login);
            return -1;
        } else {
            //printf("Wyslano komunikat o odrzuceniu zadania podwojnego logowania uzytkownika \"%s\"!\n", login);
            return -1;
        }

    } else {
        printf("Logowanie uzytkownika \"%s\"...\n", login);

        if (serwer_dodaj_uzytkownika_do_tablicy(login, klient_pid, sciezka_pobierania) != 0) 
        {
            printf("Nie udalo sie zalogowac uzytkownika \"%s\"!\n", login);
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Nie udalo sie zalogowac klienta %s!\n", login);
            closelog();

            if (serwer_wyslij_komunikat(login, "LOGIN_FAIL") != 0) {
                printf("Nie udalo sie wyslac komunikatu o bledzie przy logowaniu uzytkownika \"%s\"!\n", login);
                return -1;
            } else {
                //printf("Wyslano komunikat o bledzie przy logowaniu uzytkownika \"%s\"!\n", login);
                return -1;
            }

        } else {
            printf("Pomyslnie zalogowano uzytkownika \"%s\"!\n", login);
            //printf("Logi zostaly zapisane![sudo tail -f /var/log/syslog]\n");
            setlogmask(LOG_UPTO(LOG_NOTICE));
            openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            syslog(LOG_NOTICE, "Zalogowano pomyslnie klienta \"%s\" ! \n", login);
            closelog();
            if (serwer_wyslij_komunikat(login, "LOGIN_SUCC") != 0) {
                printf("Nie udalo sie wyslac komunikatu o pomyslnym logowaniu uzytkownika \"%s\"!\n", login);
                return -1;
            } else {
                //printf("Wyslano komunikat o pomyslnym logowaniu uzytkownika \"%s\"!\n", login);
                return 0;
            }

        }
    }
}

int serwer_dodaj_uzytkownika_do_tablicy(char *nazwa_uzytkownika, int klient_pid, char *sciezka_pobierania) {
    for (int i = 0; i < NUMBER_OF_USERS; i++) {
        if (strcmp(uzytkownicy[i], nazwa_uzytkownika) == 0) {
            printf("Jest juz taki uzytkownik w tablicy! Podwojne logowanie!\n");
            return -1;
        } else if (strcmp(uzytkownicy[i], "") == 0) {
            if (strcmp(strcpy(uzytkownicy[i], nazwa_uzytkownika), nazwa_uzytkownika) == 0) {
                if (strcmp(strcpy(download[i], sciezka_pobierania), sciezka_pobierania) == 0) {
                    pidy[i] = klient_pid;
                    //sukces
                    return 0;
                } else {
                    printf("Nie udalo sie skopiowac sciezki pobierania logowanego uzytkownika do tablicy!\n");
                    return -1;
                }
            } else {
                printf("Nie udalo sie skopiowac nazwy logowanego uzytkownika do tablicy!\n");
                return -1;
            }
        }
    }
    printf("Nie udalo sie dodac uzytkownika do tablicy (brak miejsca w tablicy)\n");
    return -1;
}

int serwer_wyslij_ramke(int fd_fifo_klient_WRITE, char *ramka) {
    if (strlen(ramka) >= FRAME_LENGTH) {
        printf("Ramka \"%s\" jest zbyt dluga by ja wyslac (%ld / %d)\n", ramka, (strlen(ramka) + 1), FRAME_LENGTH);
        return -1;
    }

    //printf("Wysylanie ramki: \"%s\" \n", ramka);

    if (write(fd_fifo_klient_WRITE, ramka, strlen(ramka)) == -1) {
        perror("Nie udalo sie wyslac ramki przez serwer!");
        return -1;
    } else {
        //printf("Wyslano ramke!\n");
        return 0;
    }
}

int serwer_wyslij_komunikat(char *odbiorca, char *wiadomosc) {
    int fd_fifo_klient_WRITE;
    char ramka[FRAME_LENGTH];
    strcpy(fifo_client_path, "potoki/");
    strcat(fifo_client_path, odbiorca);
    strcat(fifo_client_path, ".fifo");
    //printf("Szukanie potoku uzytkownika %s...\n", odbiorca);

    if (access(fifo_client_path, F_OK) == 0) {
        printf("Znaleziono potok uzytkonika %s: %s\n", odbiorca, fifo_client_path);
    } else {
        printf("Nie znaleziono potoku uzytkonika %s: %s\n", odbiorca, fifo_client_path);
        return -1;
    }

    fd_fifo_klient_WRITE = open(fifo_client_path, O_WRONLY);

    if (fd_fifo_klient_WRITE < 0) {
        perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie zapisu przez proces serwera!");
        return -1;
    }

    strcpy(ramka, "/alert ");
    strcat(ramka, wiadomosc);
    //printf("Wyslanie ramki alertu: %s\n", ramka);


    if (serwer_wyslij_ramke(fd_fifo_klient_WRITE, ramka) != 0) {
        printf("Nie udalo sie wyslac do klienta ramki z komunikatem!\n");
        return -1;
    }

    if (close(fd_fifo_klient_WRITE) == -1) {
        perror("Nie udalo sie zamknac pliku FIFO klienta przez proces serwera!");
        return -1;
    }

    return 0;
}

int serwer_wyslij_wiadomosc(char *komenda, char *nadawca, char *odbiorca, char *wiadomosc) {
    int fd_fifo_klient_WRITE;
    char ramka[FRAME_LENGTH];

    if (serwer_sprawdz_czy_uzytkownik_jest_w_tablicy_uzytkownikow(odbiorca) == 0) {
        strcpy(fifo_client_path, "potoki/");
        strcat(fifo_client_path, odbiorca);
        strcat(fifo_client_path, ".fifo");
        //printf("Szukanie potoku uzytkownika %s...\n", odbiorca);

        if (access(fifo_client_path, F_OK) == 0) {
            //printf("Znaleziono potok uzytkonika %s: %s\n", odbiorca, fifo_client_path);
        } else {
            printf("Nie znaleziono potoku uzytkonika %s!\n", odbiorca);
            return -1;
        }

        fd_fifo_klient_WRITE = open(fifo_client_path, O_WRONLY);

        if (fd_fifo_klient_WRITE < 0) {
            perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie zapisu przez proces serwera!");
            return -1;
        }

        strcpy(ramka, komenda);
        strcat(ramka, " ");
        strcat(ramka, nadawca);
        strcat(ramka, " ");
        strcat(ramka, odbiorca);
        strcat(ramka, " ");
        strcat(ramka, wiadomosc);

        if (serwer_wyslij_ramke(fd_fifo_klient_WRITE, ramka) != 0) {
            printf("Nie udalo sie wyslac do klienta ramki z wiadomoscia!\n");
            return -1;
        }

        if (close(fd_fifo_klient_WRITE) == -1) {
            perror("Nie udalo sie zamknac pliku FIFO klienta przez serwer!");
            return -1;
        }

        return 0;

    } else {
        printf("Probowano wyslac wiadomosc do niezalogowanego uzytkownika (od \"%s\" do \"%s\")!\n!", nadawca, odbiorca);
        return -1;
    }
}


