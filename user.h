#ifndef USER_H
#define USER_H

void user_login(char *user, char *download_path, char *server_path);
void user_read();
void user_sending();
void handler_SIGINT_user(int signum);
void handler_SIGQUIT_user(int signum);
void handler_SIGUSR1_user(int signum);
void handler_SIGCHLD_user_mother(int signum); 
void handler_SIGINT_serwer(int signum);

#endif

