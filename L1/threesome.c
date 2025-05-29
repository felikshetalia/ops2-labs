#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t keepRunning = 1;

void handle_sigint(int sig) {
    keepRunning = 0;
}
void usage(char *name){
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "0<n<=10 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv){
    srand(time(NULL));
    int pc1[2], c1c2[2], c2p[2];
    if(pipe(pc1) < 0 || pipe(c1c2) < 0 || pipe(c2p) < 0){
        ERR("pipe");
    }
    int randval = rand() % 21 - 10;
    int randnum = rand() % 99;
    signal(SIGINT, handle_sigint);
    pid_t pid;
    while(keepRunning){
        pid = fork();
        switch (pid)
        {
            case -1:
                ERR("fork(): ");
            case 0:
                // 1st child
                int tmp1;
                if(read(pc1[0], &tmp1, sizeof(int)) < 0){
                    ERR("read from parent");
                }
                else{
                    printf("I am child with PID [%d] and I got number %d \n", getpid(), tmp1);
                }
                // close after child reads from parent
                if(close(pc1[0]) < 0){
                    ERR("close");
                }
                tmp1 += randval;
                if(write(c1c2[1], &tmp1, sizeof(int)) < 0){
                    ERR("write to other child");
                }
                else{
                    printf("Child1 with PID [%d] sends %d \n", getpid(), tmp1);
                }
                if(close(c1c2[1]) < 0){
                    ERR("close");
                }
                exit(0);
                
        }

        pid = fork();
        switch (pid)
        {
            case -1:
                ERR("fork");
            case 0:
                // 2nd child
                int tmp2;
                if(read(c1c2[0], &tmp2, sizeof(int)) < 0){
                    ERR("read from other child");
                }
                else{
                    printf("I am child with PID [%d] and I got number %d\n", getpid(), tmp2);
                }
                if(close(c1c2[0]) < 0){
                    ERR("close");
                }
                randval = rand() % 21 - 10;
                tmp2 += randval;
                if(write(c2p[1], &tmp2, sizeof(int)) < 0){
                    ERR("write to parent");
                }
                else{
                    printf("Child2 with PID [%d] sends %d \n", getpid(), tmp2);
                }
                if(close(c2p[1]) < 0){
                    ERR("close");
                }
                exit(0);
        }

        // parent

        if(write(pc1[1], &randnum, sizeof(int)) < 0){
            ERR("write to child");
        }
        else{
            printf("Parent with PPID [%d] sent child1 the number %d\n", getppid(), randnum);
        }
        if(close(pc1[1]) < 0){
            ERR("close");
        }
        int tmp3;
        if(read(c2p[0], &tmp3, sizeof(int)) < 0){
            ERR("read from child");
        }
        else{
            printf("Parent with PPID [%d] reads %d\n", getppid(), tmp3);
        }
        if(close(c2p[0]) < 0){
            ERR("close");
        }
    }
    while(wait(NULL) > 0);
    return 0;
}