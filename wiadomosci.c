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

#define FIFO_FOLDER "potoki/"
#define FIFO_SERVER_FILE "serwer.fifo"
#define LOCK_FILE "potoki/server.pid"
#define PATH_LENGTH 50
#define FRAME_LENGTH 400
#define NUMBER_OF_USERS 5
#define USERNAME_LENGTH 25
#define COMMAND_LENGTH 10


int podziel_ramke_1(char *src, char *dest_command) 
{
    char temp[FRAME_LENGTH];
    ///printf("%s %s sprawdzam co nie tak ", src , dest_command);

    if (strcmp(strcpy(temp, src), src) == 0) 
    {
       // printf("poprawnie tymczasowo skopiowano tablice ramki\n");
    } 
    else 
    {
        printf("nie udalo sie tymczasowo skopiowac tablicy ramki\n");
        return -1;
    }

    char *token = strtok(temp, " "); // przeczytaj komende
    
    //printf("%s token \n", token);

    if (token != NULL) 
    {
        //printf("token z komenda: \"%s\" \n", token);

        if (strlen(token) > (COMMAND_LENGTH + 1)) 
        {
            printf("komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        } 
        else 
        {

            if (strcmp(strcpy(dest_command, token), token) == 0) 
            {
             //   printf("skopiowano token z komenda: \"%s\" \n", token);
                return 0;
            } 
            else 
            {
                perror("nie udalo sie skopiowac tokenu z komenda");
                return -1;
            }

        }

    } 
    else 
    {
        perror("nie udalo sie odczytac komendy z ramki (token z komenda = NULL)");
        return -1;
    }
   }
   // Dzielenie ramki na serwerze - login, logout
   int podziel_ramke_2(char *src, char *dest_command, char *dest_sender) { //wywolywana jesli ramka zaczyna sie od znaku '/'
    char temp[FRAME_LENGTH];
    if (strcmp(strcpy(temp, src), src) == 0) {
        //printf("poprawnie tymczasowo skopiowano tablice ramki\n");
    } else {
        printf("nie udalo sie tymczasowo skopiowac tablicy ramki\n");
        return -1;
    }

    char *token = strtok(temp, " "); // przeczytaj komende
    if (token != NULL) {
        //printf("token z komenda: \"%s\" \n", token);
        if (strlen(token) > (COMMAND_LENGTH + 1)) {
            printf("komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        } else {
            if (strcmp(strcpy(dest_command, token), token) == 0) {
                //printf("skopiowano token z komenda: \"%s\" \n", token);
            } else {
                perror("nie udalo sie skopiowac tokenu z komenda");
                return -1;
            }

            char *token = strtok(NULL, "\0"); // przeczytaj nadawce
            if (token != NULL) {
                //printf("token z nadawca: \"%s\" \n", token);
                if (strlen(token) > (USERNAME_LENGTH + 1)) {
                    printf("nazwa nadawcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                           USERNAME_LENGTH + 1);
                    return -1;
                } else {
                    if (strcmp(strcpy(dest_sender, token), token) == 0) {
                        //printf("skopiowano token z nadawca: \"%s\" \n", token);
                        return 0;
                    } else {
                        perror("nie udalo sie skopiowac tokenu z nadawca");
                        return -1;
                    }
                }
            } else {
                perror("nie udalo sie odczytac nadawcy z ramki (token z nadawca = NULL)");
                return -1;
            }
        }
    } else {
        perror("nie udalo sie odczytac komendy z ramki (token z komenda = NULL)");
        return -1;
    }
}
// Dzielenie polecenia na kliencie - msg, file; Dzielenie ramki na serwerze - login ze sciezka pobierania
int podziel_ramke_3(char *src, char *dest_command, char *dest_receiver, char *dest_content) {
    char temp[FRAME_LENGTH];
    if (strcmp(strcpy(temp, src), src) == 0) {
        //printf("poprawnie tymczasowo skopiowano tablice ramki\n");
    } else {
        printf("nie udalo sie tymczasowo skopiowac tablicy ramki\n");
        return -1;
    }
    char *token = strtok(temp, " "); // przeczytaj komende
    
    if (token != NULL) {
        //printf("token z komenda: \"%s\" \n", token);
        
        if (strlen(token) > (COMMAND_LENGTH + 1)) {
            printf("komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        } else {
            if (strcmp(strcpy(dest_command, token), token) == 0) {
                //printf("skopiowano token z komenda: \"%s\" \n", token);
            } else {
                perror("nie udalo sie skopiowac tokenu z komenda");
                return -1;
            }
            
            if(strcmp(token,"/users")==0)
        {
        strcat(temp," a a");
        }

            char *token = strtok(NULL, " "); // przeczytaj odbiorce
            if (token != NULL) {
                //printf("token z odbiorca: \"%s\" \n", token);
                if (strlen(token) > (USERNAME_LENGTH + 1)) {
                    printf("nazwa odbiorcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                           USERNAME_LENGTH + 1);
                    return -1;
                } else {
                    if (strcmp(strcpy(dest_receiver, token), token) == 0) {
                      //  printf("skopiowano token z odbiorca: \"%s\" \n", token);
                    } else {
                        perror("nie udalo sie skopiowac tokenu z odbiorca");
                        return -1;
                    }

                    token = strtok(NULL, "\0"); // przeczytaj reszte ramki, czyli content
                 //   printf("token z trescia ramki: \"%s\" \n", token);
                    if (token != NULL) {
                        if (strlen(token) > (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)) {
                            printf("tresc ramki \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2));
                            return -1;
                        } else {
                            if (strcmp(strcpy(dest_content, token), token) == 0) {
                                //printf("skopiowano token z trescia ramki: \"%s\"\n", token);
                                return 0;
                            } else {
                                perror("nie udalo sie skopiowac tokenu z trescia ramki");
                                return -1;
                            }
                        }
                    } else {
                        strcpy(dest_content, "");
                        perror("nie udalo sie odczytac contentu z ramki (token z contentem = NULL)");
                        return -1;
                    }
                }
            } else {
                strcpy(dest_receiver, "");
                perror("nie udalo sie odczytac odbiorcy z ramki (token z odbiorca = NULL)");
                return -1;
            }
        }
    } else {
        strcpy(dest_command, "");
        perror("nie udalo sie odczytac komendy z ramki (token z komenda = NULL)");
        return -1;
    }
}
   // Dzielenie ramki na serwerze - msg, file
   int podziel_ramke_4(char *src, char *dest_command, char *dest_sender, char *dest_receiver, char *dest_content) {
    char temp[FRAME_LENGTH];
    if (strcmp(strcpy(temp, src), src) == 0) {
       // printf("poprawnie tymczasowo skopiowano tablice ramki\n");
    } else {
        printf("nie udalo sie tymczasowo skopiowac tablicy ramki\n");
        return -1;
    }
    char *token = strtok(temp, " "); // przeczytaj komende
    if (token != NULL) {
        //printf("token z komenda: \"%s\" \n", token);
        if (strlen(token) > (COMMAND_LENGTH + 1)) {
            printf("komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        } else {
            if (strcmp(strcpy(dest_command, token), token) == 0) {
                //printf("skopiowano token z komenda: \"%s\" \n", token);
            } else {
                perror("nie udalo sie skopiowac tokenu z komenda");
                return -1;
            }

            char *token = strtok(NULL, " ");
            if (token != NULL) {
                //printf("token z nadawca: \"%s\" \n", token);
                if (strlen(token) > (USERNAME_LENGTH + 1)) {
                    printf("nazwa nadawcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                           USERNAME_LENGTH + 1);
                    return -1;
                } else {
                    if (strcmp(strcpy(dest_sender, token), token) == 0) {
                        //printf("skopiowano token z nadawca: \"%s\" \n", token);
                    } else {
                        perror("nie udalo sie skopiowac tokenu z nadawca");
                        return -1;
                    }

                    char *token = strtok(NULL, " ");
                    if (token != NULL) {
                        //printf("token z odbiorca: \"%s\" \n", token);
                        if (strlen(token) > (USERNAME_LENGTH + 1)) {
                            printf("nazwa odbiorcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                                   USERNAME_LENGTH + 1);
                            return -1;
                        } else {
                            if (strcmp(strcpy(dest_receiver, token), token) == 0) {
                                //printf("skopiowano token z odbiorca: \"%s\" \n", token);
                            } else {
                                perror("nie udalo sie skopiowac tokenu z odbiorca");
                                return -1;
                            }

                            token = strtok(NULL, "\0");
                         //   printf("token z trescia ramki: \"%s\" \n", token);
                            if (token != NULL) {
                                if (strlen(token) > (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2)) {
                                    printf("tresc ramki \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                                           (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2));
                                    return -1;
                                } else {
                                    if (strcmp(strcpy(dest_content, token), token) == 0) {
                                        //printf("skopiowano token z trescia ramki: \"%s\"\n", token);
                                        return 0;
                                    } else {
                                        perror("nie udalo sie skopiowac tokenu z trescia ramki");
                                        return -1;
                                    }
                                }
                            } else {
                                strcpy(dest_content, "");
                                perror("nie udalo sie odczytac contentu z ramki (token z contentem = NULL)");
                                return -1;
                            }
                        }
                    } else {
                        strcpy(dest_receiver, "");
                        perror("nie udalo sie odczytac odbiorcy z ramki (token z odbiorca = NULL)");
                        return -1;
                    }
                }
            } else {
                strcpy(dest_sender, "");
                perror("nie udalo sie odczytac nadawcy z ramki (token z nadawca = NULL)");
                return -1;
            }
        }
    } else {
        strcpy(dest_command, "");
        perror("nie udalo sie odczytac komendy z ramki (token z komenda = NULL)");
        return -1;
    }
}
