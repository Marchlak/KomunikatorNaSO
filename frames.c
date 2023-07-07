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

#define FIFO_FOLDER "potoki/"
#define FIFO_SERVER_FILE "serwer.fifo"
#define LOCK_FILE "potoki/server.pid"
#define PATH_LENGTH 50
#define FRAME_LENGTH 400
#define NUMBER_OF_USERS 5
#define USERNAME_LENGTH 25
#define COMMAND_LENGTH 10


//dzielenia ramki z samą komendą
int frame_splitting_command(char *source, char *frame_command)
{
    char temp[FRAME_LENGTH];
    #ifdef DEBUG
     printf("%s %s zrodlo i komenda", source , frame_command);
    #endif
    if (strcmp(strcpy(temp, source), source) == 0)
    {
         #ifdef DEBUG
         printf("Poprawnie tymczasowo skopiowano tablice ramki\n");
         #endif
    }
    else
    {
        printf("Nie udalo sie skopiowac ramki\n");
        return -1;
    }

    char *token = strtok(temp, " "); // przeczytaj komende
    #ifdef DEBUG
     printf("%s token \n", token);
    #endif
    if (token != NULL)
    {
        #ifdef DEBUG   
         printf("token z komenda: \"%s\" \n", token);
        #endif

        if (strlen(token) > (COMMAND_LENGTH + 1))
        {
            printf("Komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        }
        else
        {

            if (strcmp(strcpy(frame_command, token), token) == 0)
            {
            #ifdef DEBUG
                   printf("skopiowano token z komenda: \"%s\" \n", token);
            #endif
                return 0;
            }
            else
            {
                perror("Nie udalo sie skopiowac tokenu z komenda");
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
// Dzielenie ramki na serwerze - login, logout komenda i wysylajacy
int frame_splitting_command_sender(char *source, char *frame_command, char *frame_sender)
{ // wywolywana jesli ramka zaczyna sie od znaku '/'
    char temp[FRAME_LENGTH];
    if (strcmp(strcpy(temp, source), source) == 0)
    {
        // printf("poprawnie tymczasowo skopiowano tablice ramki\n");
    }
    else
    {
        printf("nie udalo sie tymczasowo skopiowac tablicy ramki\n");
        return -1;
    }

    char *token = strtok(temp, " "); // przeczytaj komende
    if (token != NULL)
    {
        // printf("token z komenda: \"%s\" \n", token);
        if (strlen(token) > (COMMAND_LENGTH + 1))
        {
            printf("komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        }
        else
        {
            if (strcmp(strcpy(frame_command, token), token) == 0)
            {
                // printf("skopiowano token z komenda: \"%s\" \n", token);
            }
            else
            {
                perror("nie udalo sie skopiowac tokenu z komenda");
                return -1;
            }

            char *token = strtok(NULL, "\0"); // przeczytaj nadawce
            if (token != NULL)
            {
                // printf("token z nadawca: \"%s\" \n", token);
                if (strlen(token) > (USERNAME_LENGTH + 1))
                {
                    printf("nazwa nadawcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                           USERNAME_LENGTH + 1);
                    return -1;
                }
                else
                {
                    if (strcmp(strcpy(frame_sender, token), token) == 0)
                    {
                        // printf("skopiowano token z nadawca: \"%s\" \n", token);
                        return 0;
                    }
                    else
                    {
                        perror("nie udalo sie skopiowac tokenu z nadawca");
                        return -1;
                    }
                }
            }
            else
            {
                perror("nie udalo sie odczytac nadawcy z ramki (token z nadawca = NULL)");
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
// Dzielenie polecenia na kliencie - msg, file; Dzielenie ramki na serwerze - login ze sciezka pobierania
int frame_splitting_command_receiver_message(char *source, char *frame_command, char *frame_receiver, char *frame_message)
{
    char temp[FRAME_LENGTH];
    if (strcmp(strcpy(temp, source), source) == 0)
    {
         #ifdef DEBUG
         printf("poprawnie tymczasowo skopiowano tablice ramki\n");
         #endif
    }
    else
    {
        printf("nie udalo sie tymczasowo skopiowac tablicy ramki\n");
        return -1;
    }
    char *token = strtok(temp, " "); // przeczytaj komende

    if (token != NULL)
    {
         #ifdef DEBUG
         printf("token z komenda: \"%s\" \n", token);
         #endif
        if (strlen(token) > (COMMAND_LENGTH + 1))
        {
            printf("komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        }
        else
        {
            if (strcmp(strcpy(frame_command, token), token) == 0)
            {
                 #ifdef DEBUG           
                 printf("skopiowano token z komenda: \"%s\" \n", token);
                 #endif
            }
            else
            {
                perror("nie udalo sie skopiowac tokenu z komenda");
                return -1;
            }

            if (strcmp(token, "/users") == 0) //za krotka ramka glupie ale dziala
            {
                strcat(temp, " BRAK BRAK");
            }

            char *token = strtok(NULL, " "); // przeczytaj odbiorce
            if (token != NULL)
            {
                   #ifdef DEBUG
                 printf("token z odbiorca: \"%s\" \n", token);
                   #endif
                if (strlen(token) > (USERNAME_LENGTH + 1))
                {
                    printf("nazwa odbiorcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                           USERNAME_LENGTH + 1);
                    return -1;
                }
                else
                {
                    if (strcmp(strcpy(frame_receiver, token), token) == 0)
                    {
                          #ifdef DEBUG
                          printf("skopiowano token z odbiorca: \"%s\" \n", token);
                          #endif
                    }
                    else
                    {
                        perror("nie udalo sie skopiowac tokenu z odbiorca");
                        return -1;
                    }

                    token = strtok(NULL, "\0"); // przeczytaj reszte ramki, czyli wiadomosc
                      #ifdef DEBUG
                      printf("token z trescia ramki: \"%s\" \n", token);
                      #endif
                    if (token != NULL)
                    {
                        if (strlen(token) > (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2))
                        {
                            printf("tresc ramki \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2));
                            return -1;
                        }
                        else
                        {
                            if (strcmp(strcpy(frame_message, token), token) == 0)
                            {
                            #ifdef DEBUG
                                printf("skopiowano token z trescia ramki: \"%s\"\n", token);
                            #endif
                                return 0;
                            }
                            else
                            {
                                perror("Blad przy kopiowaniu ramki");
                                return -1;
                            }
                        }
                    }
                    else
                    {
                        strcpy(frame_message, "");
                        perror("Blad przy odczytaniu zwartosci ramki (token bez contentu)");
                        return -1;
                    }
                }
            }
            else
            {
                strcpy(frame_receiver, "");
                perror("Blad przy odczytaniu odbiorcy z ramki (NULL)");
                return -1;
            }
        }
    }
    else
    {
        strcpy(frame_command, "");
        perror("Blad przy komendy z ramki (NULL)");
        return -1;
    }
}
// Dzielenie ramki na serwerze - msg, file
int frame_splitting_command_sender_receiver_message(char *source, char *frame_command, char *frame_sender, char *frame_receiver, char *frame_message)
{
    char temp[FRAME_LENGTH];
    if (strcmp(strcpy(temp, source), source) == 0)
    {
         #ifdef DEBUG
         printf("poprawnie tymczasowo skopiowano tablice ramki\n");
         #endif
    }
    else
    {
        printf("nie udalo sie tymczasowo skopiowac tablicy ramki\n");
        return -1;
    }
    char *token = strtok(temp, " "); // przeczytaj komende
    if (token != NULL)
    {
         #ifdef DEBUG
         printf("token z komenda: \"%s\" \n", token);
         #endif
        if (strlen(token) > (COMMAND_LENGTH + 1))
        {
            printf("komenda \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token), COMMAND_LENGTH + 1);
            return -1;
        }
        else
        {
            if (strcmp(strcpy(frame_command, token), token) == 0)
            {
                 #ifdef DEBUG
                 printf("skopiowano token z komenda: \"%s\" \n", token);
                 #endif
            }
            else
            {
                perror("nie udalo sie skopiowac tokenu z komenda");
                return -1;
            }

            char *token = strtok(NULL, " ");
            if (token != NULL)
            {
                // printf("token z nadawca: \"%s\" \n", token);
                if (strlen(token) > (USERNAME_LENGTH + 1))
                {
                    printf("nazwa nadawcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                           USERNAME_LENGTH + 1);
                    return -1;
                }
                else
                {
                    if (strcmp(strcpy(frame_sender, token), token) == 0)
                    {
                         #ifdef DEBUG
                         printf("skopiowano token z nadawca: \"%s\" \n", token);
                         #endif
                    }
                    else
                    {
                        perror("nie udalo sie skopiowac tokenu z nadawca");
                        return -1;
                    }

                    char *token = strtok(NULL, " ");
                    if (token != NULL)
                    {    
                         #ifdef DEBUG
                         printf("token z odbiorca: \"%s\" \n", token);
                         #endif
                        if (strlen(token) > (USERNAME_LENGTH + 1))
                        {
                            printf("nazwa odbiorcy \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                                   USERNAME_LENGTH + 1);
                            return -1;
                        }
                        else
                        {
                            if (strcmp(strcpy(frame_receiver, token), token) == 0)
                            {
                                #ifdef DEBUG
                                 printf("skopiowano token z odbiorca: \"%s\" \n", token);
                                 #endif
                            }
                            else
                            {
                                perror("nie udalo sie skopiowac tokenu z odbiorca");
                                return -1;
                            }

                            token = strtok(NULL, "\0");
                               #ifdef DEBUG
                               printf("token z trescia ramki: \"%s\" \n", token);
                               #endif
                            if (token != NULL)
                            {
                                if (strlen(token) > (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2))
                                {
                                    printf("tresc ramki \"%s\" jest zbyt dluga (%ld/%d)\n", token, strlen(token),
                                           (FRAME_LENGTH - USERNAME_LENGTH - USERNAME_LENGTH - 2));
                                    return -1;
                                }
                                else
                                {
                                    if (strcmp(strcpy(frame_message, token), token) == 0)
                                    {
                                         #ifdef DEBUG
                                         printf("skopiowano token z trescia ramki: \"%s\"\n", token);
                                         #endif
                                        return 0;
                                    }
                                    else
                                    {
                                        perror("nie udalo sie skopiowac tokenu z trescia ramki");
                                        return -1;
                                    }
                                }
                            }
                            else
                            {
                                strcpy(frame_message, "");
                                perror("nie udalo sie odczytac contentu z ramki (token z contentem = NULL)");
                                return -1;
                            }
                        }
                    }
                    else
                    {
                        strcpy(frame_receiver, "");
                        perror("nie udalo sie odczytac odbiorcy z ramki (token z odbiorca = NULL)");
                        return -1;
                    }
                }
            }
            else
            {
                strcpy(frame_sender, "");
                perror("nie udalo sie odczytac nadawcy z ramki (token z nadawca = NULL)");
                return -1;
            }
        }
    }
    else
    {
        strcpy(frame_command, "");
        perror("nie udalo sie odczytac komendy z ramki (token z komenda = NULL)");
        return -1;
    }
}
