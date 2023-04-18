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
#include"serwer.h"

#define _POSIX_C_SOURCE 200809L

int main(int argc, char **argv)
{
    int option_cases; //wybrana opcja
    int digit_optind = 0;
    int this_option_optind = optind ? optind : 1;
    int option_index = 0; //indeks opcji
    int chosen_option = 0; //czy zostala wybrana opcja
    int opt_l_count = 0; // liczba podanych argumentów dla opcji -l
    int opt_d_count = 0; // liczba podanych argumentów dla opcji -d

    static struct option options_long[] = {
        {"serwer", no_argument, 0, 's'},
        {"login", required_argument, 0, 'l'},
        {"download", required_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    while ((option_cases = getopt_long(argc, argv, "sd:l:", options_long, &option_index)) != -1) {
        switch (option_cases) {
            case 's':
                if (chosen_option) {
                    fprintf(stderr, "Error: Możesz wybrac tylko jedna opcje naraz\n");
                    return EXIT_FAILURE;
                }
                chosen_option = 's';
                printf("serwer \n");
                struct sigaction sa;
    		sa.sa_handler = signal_handler;
   	        sigemptyset(&sa.sa_mask);
                sa.sa_flags = SA_RESTART;
                sigaction(SIGINT, &sa, NULL);
                sigaction(SIGTERM, &sa, NULL);
                sigaction(SIGTSTP, &sa, NULL);
                serwer_start();
                break;
            case 'l':
                if (chosen_option) {
                    fprintf(stderr, "Error: Mozesz wybrac tylko jedna opcje naraz.\n");
                    return EXIT_FAILURE;
                }
                 if(opt_l_count > 0)
                {
                fprintf(stderr, "Opcja -l może przymować tylko jeden argument");
                exit(EXIT_FAILURE);
                }
                chosen_option = 'l';
                
                printf("login \n");
                if (optarg) {
                    printf("with arg %s\n", optarg);
                }
                break;
            case 'd':
                if (chosen_option) {
                    fprintf(stderr, "Error: Mozesz wybrac tylko jedna opcje naraz.\n");
                    return EXIT_FAILURE;
                }
                if(opt_l_count > 0)
                {
                fprintf(stderr, "Opcja -l może przymować tylko jeden argument");
                exit(EXIT_FAILURE);
                }
                chosen_option = 'd';
                printf("download \n");
                if (optarg) {
                    printf("Zalogowano uzytkownika %s\n", optarg);
                }
                break;
            case '?':
            printf("bład przy wyborze opcji zakończenie programu");
            //dopisac zeby cos do logow wrzucalo
                break;
        }
    }
    //komentarz
    return EXIT_SUCCESS;
}
