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
#define INT_DIGITS 19

char username[(USERNAME_LENGTH + 1)];
char fifo_client_path[PATH_LENGTH];
char download_path[PATH_LENGTH] = "";
int fd_serwer_fifo_WRITE;
char fifo_server_path[PATH_LENGTH];
int fd_serwer_fifo_WRITE;
int fd_fifo_klient_READ;

char* itoa(i) //funkcja itoa wzięta z netu
        int i;
{
    static char buf[INT_DIGITS + 2];
    char *p = buf + INT_DIGITS + 1;
    if (i >= 0) {
        do {
            *--p = '0' + (i % 10);
            i /= 10;
        } while (i != 0);
        return p;
    } else {
        do {
            *--p = '0' - (i % 10);
            i /= 10;
        } while (i != 0);
        *--p = '-';
    }
    return p;
}

void extract_filename(char* path, char* filename) {
    // znajdź pozycję ostatniego wystąpienia znaku '/'
    char* last_slash = strrchr(path, '/');
    
    // jeśli slash nie został znaleziony, skopiuj całą ścieżkę
    if (last_slash == NULL) {
        strcpy(filename, path);
        return;
    }
    
    // skopiuj nazwę pliku bez slash'a
    strcpy(filename, last_slash + 1);
}


void klient_pobieranie(char *source, char *destination)
{

   int fd_src, fd_dst;
    struct stat stat_src;
    off_t offset = 0;
    char filename[1000];
    extract_filename(source,filename);
    strcat(destination, "/");
    strcat(destination,filename);
    printf("%s %s \n", source, destination);

    // Otwórz plik źródłowy w trybie do odczytu
    fd_src = open(source, O_RDONLY);
    if (fd_src == -1) {
        perror("Nie udało się otworzyć pliku źródłowego");
        return;
    }

    // Pobierz statystyki pliku źródłowego
    if (fstat(fd_src, &stat_src) == -1) {
        perror("Nie udało się pobrać informacji o pliku źródłowym");
        close(fd_src);
        return;
    }

    // Otwórz plik docelowy w trybie do zapisu
    fd_dst = open(destination, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_dst == -1) {
        perror("Nie udało się otworzyć pliku docelowego");
        close(fd_src);
        return;
    }

    // Skopiuj zawartość pliku źródłowego do pliku docelowego za pomocą funkcji sendfile
    if (sendfile(fd_dst, fd_src, &offset, stat_src.st_size) == -1) {
        perror("Nie udało się skopiować pliku");
        close(fd_src);
        close(fd_dst);
        return;
    }

    // Zamknij pliki
    close(fd_src);
    close(fd_dst);
  
}


void klient_zaloguj(char *user, char *download_path_a, char *server_path)
{

    strcpy(fifo_server_path,server_path); // niepotrzebne
    strcpy (download_path_a,download_path);
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
    
     fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);
    
    if (fd_serwer_fifo_WRITE < 0) {
        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta - w celu zalogowania! Wylaczanie... \n \n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta \"%s\" - w celu zalogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }
    
    strcpy(ramka_logowania, "/login ");
    strcat(ramka_logowania, username);
    strcat(ramka_logowania, " ");
    strcat(ramka_logowania, itoa(klient_pid));
    strcat(ramka_logowania, " ");
    strcat(ramka_logowania, download_path);
    ramka_logowania[FRAME_LENGTH - 1] = '\0';
    
    printf("Ramka logowania do wyslania: %s\n", ramka_logowania);
    
    if (klient_wyslij_ramke(ramka_logowania) != 0) {
        printf("Nie udalo sie wyslac ramki logowania do serwera! Wylaczanie...\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie wyslac ramki logowania do FIFO serwera - przez proces klienta \"%s\" - w celu zalogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }
    
    fd_fifo_klient_READ = open(fifo_client_path, O_RDONLY);

    
    if (fd_fifo_klient_READ < 0) {
        perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta - w celu odebrania komunikatu dot. logowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta \"%s\" - w celu odebrania komunikatu dot. logowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }
     
    read_bytes = read(fd_fifo_klient_READ, readbuf, sizeof(readbuf));
    
    if (read_bytes < 0) {
        perror("Nie udalo sie odczytac odpowiedzi serwera na ramke logowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie odczytac odpowiedzi serwera na ramke logowania klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (close(fd_serwer_fifo_WRITE) == -1) {
        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO serwera przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (close(fd_fifo_klient_READ) == -1) {
        perror("Nie udalo sie zamknac pliku FIFO klienta przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO klienta przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    
    readbuf[read_bytes] = '\0';
    
    if (podziel_ramke_1(readbuf, komenda) == 0) {
        if (strcmp(komenda, "/alert") == 0) {
            if (podziel_ramke_2(readbuf, komenda, wiadomosc) == 0) {
                printf("pomyslnie podzielono ramke alert! - ramka: \"%s\" \n", readbuf);
                if ((strcmp(komenda, "") != 0) && (strcmp(wiadomosc, "") != 0)) {

                    if (strcmp(wiadomosc, "LOGIN_SUCC") == 0) {
                        printf("Zalogowano pomyslnie jako \"%s\"! \n", username);
                    } else if (strcmp(wiadomosc, "LOGIN_FAIL") == 0) {
                        printf("Serwer odrzucil zadanie zalogowania jako \"%s\"! Wylaczanie...\n", username);
                        exit(EXIT_FAILURE);
                    } else {
                        printf("Nieprawidlowa tresc alertu: \"%s\"! Wylaczanie...\n", wiadomosc);
                    }

                } else {
                    printf("Nieprawidlowa ramka typu alert bedaca odpowiedzia serwera na zadanie zalogowania! Wylaczanie...\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                printf("Blad podczas dzielenia ramki typu alert bedacej odpowiedzia serwera na zadanie zalogowania! Wylaczanie...\n");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Nieprawidlowa odpowiedz serwera na zadanie zalogowania! Wylaczanie...\n");
            exit(EXIT_FAILURE);
        }
    }
    
    else {
        printf("Blad wyodrebniania komendy z ramki odpowiedzi serwera na zadanie zalogowania! Wylaczanie...\n");
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
    
    int klient_usuwanie_pliku_fifo() {
    if (remove(fifo_client_path) == 0) {
        //printf("Plik FIFO uzytkownika %s zostal usuniety\n", username);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Usunieto potok klienta %s: %s\n", username, fifo_client_path);
        closelog();
        return 0;
    } else {
        printf("Nie udalo sie usunac pliku FIFO uzytkownika %s\n!", username);
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie usunac potoku klienta %s: %s\n", username, fifo_client_path);
        closelog();
        return -1;
    }
}

int klient_wyslij_ramke(char *msg) {
    if (strlen(msg) >= FRAME_LENGTH) {
        printf("Ramka \"%s\" jest zbyt dluga by ja wyslac (%ld / %d)\n", msg, (strlen(msg) + 1), FRAME_LENGTH);
        return -1;
    }

    printf("Wysylanie ramki...\n");

    if (write(fd_serwer_fifo_WRITE, msg, strlen(msg)) == -1) {
        perror("Nie udalo sie wyslac ramki do serwera!");
        return -1;
    } else {
        printf("Wyslano ramke!\n");
        return 0;
    }
}

void klient_czytanie() {
    char readbuf[FRAME_LENGTH];
    int read_bytes;
    char komenda[(COMMAND_LENGTH + 1)];
    char nadawca[(USERNAME_LENGTH + 1)];
    char odbiorca[(USERNAME_LENGTH + 1)];
    char wiadomosc[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];

    printf("Otwarto plik FIFO klienta \"%s\" (tryb odczytu):  \n", fifo_client_path);

    while (1) {
        //printf(":");
        fd_fifo_klient_READ = open(fifo_client_path, O_RDONLY);

        if (fd_fifo_klient_READ < 0) {
            perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu przez proces klienta-dziecko!");
            exit(EXIT_FAILURE);
        }

        read_bytes = read(fd_fifo_klient_READ, readbuf, sizeof(readbuf));
        readbuf[read_bytes] = '\0';

        if (strlen(readbuf) > 0) {
            if (podziel_ramke_1(readbuf, komenda) == 0) {
                if (strcmp(komenda, "/msg") == 0) {
                    if (podziel_ramke_4(readbuf, komenda, nadawca, odbiorca, wiadomosc) == 0) {
                        if ((strcmp(komenda, "") != 0) && (strcmp(nadawca, "") != 0) && (strcmp(odbiorca, "") != 0) && (strcmp(wiadomosc, "") != 0)) {
                            if (close(fd_fifo_klient_READ) == -1) {
                                perror("Nie udalo sie zamknac pliku FIFO klienta przez proces klienta-dziecko!");
                                exit(EXIT_FAILURE);
                            }else{
                                printf("%s: \"%s\" \n: ", nadawca, wiadomosc);
                                ///printf("_ ");
                            }
                        } else {
                            printf("Nieprawidlowa ramka typu msg!\n");
                        }
                    } else {
                        printf("Blad podczas dzielenia ramki typu msg\n");
                    }
                } else if (strcmp(komenda, "/file") == 0) {
                    if (podziel_ramke_4(readbuf, komenda, nadawca, odbiorca, wiadomosc) == 0) {
                        if ((strcmp(komenda, "") != 0) && (strcmp(nadawca, "") != 0) && (strcmp(odbiorca, "") != 0) && (strcmp(wiadomosc, "") != 0)) {
                            printf("Pobieranie pliku \"%s\" od uzytkownika %s?\n", wiadomosc, nadawca);
                            klient_pobieranie(wiadomosc, download_path);

                            printf("Zakonczono pobieranie pliku!\n");
                  
                        } else {
                            printf("Nieprawidlowa ramka typu file!\n");
                        }
                    } else {
                        printf("Blad podczas dzielenia ramki typu file\n");
                    }
                } else {
                    printf("Wykryto nieprawidlowa komende! (\"%s\") \n", komenda);
                }
                
            } else {
                printf("blad wyodrebniania komendy z ramki\n");
            }
        }
    }
}

void klient_nadawanie() {
    char komenda[(COMMAND_LENGTH + 1)];
    char odbiorca[(USERNAME_LENGTH + 1)];
    char wiadomosc[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];
    char ramka[FRAME_LENGTH];
    int stringlen;
    char readbuf[FRAME_LENGTH - 5];

    while (1) {
        ///printf("_ ");
        fgets(readbuf, sizeof(readbuf), stdin);
        if (strlen(readbuf) > 1) {
            stringlen = strlen(readbuf);
            readbuf[stringlen - 1] = '\0';
            podziel_ramke_3(readbuf, komenda, odbiorca, wiadomosc);
            if ((strcmp(komenda, "") != 0) && (strcmp(odbiorca, "") != 0) && (strcmp(wiadomosc, "") != 0)) {
                if (strcmp(komenda, "/msg") == 0) {
                    strcpy(ramka, komenda);
                    strcat(ramka, " ");
                    strcat(ramka, username);
                    strcat(ramka, " ");
                    strcat(ramka, odbiorca);
                    strcat(ramka, " ");
                    strcat(ramka, wiadomosc);

                    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

                    if (fd_serwer_fifo_WRITE < 0) {
                        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta-matki - w celu wyslania ramki typu msg!");
                        setlogmask(LOG_UPTO(LOG_NOTICE));
                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta-matki - w celu wyslania ramki typu msg przez uzytkownika: %s!", username);
                        closelog();
                    }

                    if (klient_wyslij_ramke(ramka) != 0) {
                        printf("Nie udalo sie wyslac do serwera ramki typu msg!\n");
                        setlogmask(LOG_UPTO(LOG_NOTICE));
                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                        syslog(LOG_NOTICE, "Nie udalo sie wyslac do serwera ramki typu msg! przez uzytkownika: %s", username);
                        closelog();
                    }

                    if (close(fd_serwer_fifo_WRITE) == -1) {
                        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta-matke!");
                        exit(EXIT_FAILURE);
                    }

                } else if (strcmp(komenda, "/file") == 0) {
                    strcpy(ramka, komenda);
                    strcat(ramka, " ");
                    strcat(ramka, username);
                    strcat(ramka, " ");
                    strcat(ramka, odbiorca);
                    strcat(ramka, " ");
                    strcat(ramka, wiadomosc);

                    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

                    if (fd_serwer_fifo_WRITE < 0) {
                        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta-matki - w celu ramki typu file!");
                    }

                    if (klient_wyslij_ramke(ramka) != 0) {
                        printf("Nie udalo sie wyslac do serwera ramki typu file!\n");
                        setlogmask(LOG_UPTO(LOG_NOTICE));
                        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                        syslog(LOG_NOTICE, "Nie udalo sie wyslac do serwera ramki typu file!\n");
                        closelog();
                    }

                    if (close(fd_serwer_fifo_WRITE) == -1) {
                        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta-matke!");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    printf("Nie ma takiej komendy!\n");
                }
            } else {
                printf("Nieprawidlowe polecenie!\n");
            }
        } else {
            printf("Za krotka wiadomosc!\n");
        }
    }
}

void klient_wyloguj() {
    char ramka_logowania[FRAME_LENGTH] = "";
    char readbuf[FRAME_LENGTH];
    int read_bytes;
    char komenda[(COMMAND_LENGTH + 1)];
    char wiadomosc[(FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)];

    fd_serwer_fifo_WRITE = open(fifo_server_path, O_WRONLY);

    if (fd_serwer_fifo_WRITE < 0) {
        perror("Nie udalo sie otworzyc pliku FIFO serwera w trybie zapisu - przez proces klienta - w celu wylogowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO klienta w trybie zapisu - przez proces klienta \"%s\" - w celu wylogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    strcpy(ramka_logowania, "/logout ");
    strcat(ramka_logowania, username);
    ramka_logowania[FRAME_LENGTH - 1] = '\0';
    //printf("Ramka wylogowania do wyslania: %s\n", ramka_logowania);

    if (klient_wyslij_ramke(ramka_logowania) != 0) {
        printf("Nie udalo sie wyslac ramki wylogowania do serwera! Wylaczanie...\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie wyslac ramki wylogowania do serwera przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    fd_fifo_klient_READ = open(fifo_client_path, O_RDONLY);

    if (fd_fifo_klient_READ < 0) {
        perror("Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta - w celu odebrania komunikatu dot. wylogowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie otworzyc pliku FIFO klienta w trybie odczytu - przez proces klienta \"%s\" - w celu odebrania komunikatu dot. wylogowania!\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    read_bytes = read(fd_fifo_klient_READ, readbuf, sizeof(readbuf));

    if (read_bytes < 0) {
        perror("Nie udalo sie odczytac odpowiedzi serwera na ramke wylogowania! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie odczytac odpowiedzi serwera na ramke wylogowania - przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    sleep(1);

    if (close(fd_serwer_fifo_WRITE) == -1) {
        perror("Nie udalo sie zamknac pliku FIFO serwera przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO serwera przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (close(fd_fifo_klient_READ) == -1) {
        perror("Nie udalo sie zamknac pliku FIFO klienta przez proces klienta! Wylaczanie...");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie zamknac pliku FIFO klienta przez proces klienta \"%s\" !\n", username);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (klient_usuwanie_pliku_fifo() == 0) {
        printf("Pomyslnie usunieto plik FIFO klienta.\n");
    } else {
        printf("Nie udalo sie usunac pliku FIFO klienta!\n");
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog("KOMUNIKATOR:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_NOTICE, "Nie udalo sie usunac pliku FIFO klienta \"%s\" !\n", username);
        closelog();
    }

    readbuf[read_bytes] = '\0';

    if (podziel_ramke_1(readbuf, komenda) == 0) {
        if (strcmp(komenda, "/alert") == 0) {
            if (podziel_ramke_2(readbuf, komenda, wiadomosc) == 0) {
                //printf("pomyslnie podzielono ramke alert! - ramka: \"%s\" \n", readbuf);

                if ((strcmp(komenda, "") != 0) && (strcmp(wiadomosc, "") != 0)) {

                    if (strcmp(wiadomosc, "LOGOUT_SUCC") == 0) {
                        printf("Wylogowano pomyslnie! \n");
                    } else if (strcmp(wiadomosc, "LOGOUT_FAIL") == 0) {
                        printf("Serwer odrzucil zadanie wylogowania!\n");
                        exit(EXIT_FAILURE);
                    } else {
                        printf("Nieprawidlowa tresc alertu: \"%s\"! Konczenie...\n", wiadomosc);
                    }

                } else {
                    printf("Nieprawidlowa ramka typu alert bedaca odpowiedzia serwera na zadanie wylogowania! Konczenie...\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                printf("Blad podczas dzielenia ramki typu alert bedacej odpowiedzia serwera na zadanie wylogowania! Konczenie...\n");
                printf("ramka: \"%s\" \n", readbuf);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Nieprawidlowa odpowiedz serwera na zadanie wylogowania! Konczenie...\n");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Blad wyodrebniania komendy z ramki odpowiedzi serwera na zadanie wylogowania! Konczenie...\n");
        exit(EXIT_FAILURE);
    }
}


void handler_SIGINT_klient(int signum) {
    write(1, "\nProgram zakonczony przez Ctrl+C!\n", 40);
    klient_wyloguj();
    exit(signum);
}

void handler_SIGQUIT_klient(int signum) {
    write(1, "\nProgram zakonczony przez Ctrl+\\!\n", 40);
    klient_wyloguj();
    exit(signum);
}

void handler_SIGUSR1_klient(int signum) {
    printf("Otrzymano informacje od serwera, iz jest on wylaczany - wylogowano wszystkich klientow. Konczenie...\n");
    exit(EXIT_SUCCESS);
}

void handler_SIGCHLD_klient_matka(int signum) {
    write(1, "\nProces klienta-dziecka zostal zakonczony.\n", 44);
   
}






