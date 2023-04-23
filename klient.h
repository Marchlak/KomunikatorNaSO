#ifndef KLIENT_H
#define KLIENT_H

void klient_zaloguj(char *user, char *fifo_server_path, char *download_path);
void handler_SIGINT_klient(int signum);
void handler_SIGQUIT_klient(int signum);
void handler_SIGUSR1_klient(int signum);
void handler_SIGCHLD_klient_matka(int signum); 
void handler_SIGINT_serwer(int signum);

#endif

