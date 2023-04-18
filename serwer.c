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
#define LOCK_FILE "/var/run/server.pid"


void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM || signum == SIGTSTP) {
          int lock_file = open("lockfile", O_RDONLY);
        if (lock_file < 0) {
            printf("Error opening lockfile.\n");
            exit(1);
        }
        int ret = flock(lock_file, LOCK_UN);
        if (ret < 0) {
            printf("Error unlocking file.\n");
            exit(1);
        }
        close(lock_file);
        printf("Server is shutting down.\n");
        exit(0);
    
    }
}

void serwer_start()
{
printf("działa");

   int lock_file = open("lockfile", O_CREAT | O_EXCL, 0644);
    if (lock_file < 0) {
        printf("Another instance of the server is already running.\n");
        exit(1);
    }
    
    for(int i=1;i<=10;i++)
    {
    printf("serwer działa");
    sleep(1);
   }
   cleanup();
   
}
