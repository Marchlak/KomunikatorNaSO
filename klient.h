#ifndef KLIENT_H
#define KLIENT_H

void klient_zaloguj(char *user, char *download_path, char *server_path);
void klient_czytanie();
void klient_nadawanie();
void handler_SIGINT_klient(int signum);
void handler_SIGQUIT_klient(int signum);
void handler_SIGUSR1_klient(int signum);
void handler_SIGCHLD_klient_matka(int signum); 
void handler_SIGINT_serwer(int signum);

#endif

