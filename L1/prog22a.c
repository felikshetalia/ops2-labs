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


void usage(char *name){
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "0<n<=10 - number of children\n");
    exit(EXIT_FAILURE);
}
int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (0 == pid)
            return;
        if (0 >= pid)
        {
            if (ECHILD == errno)
                return;
            ERR("waitpid:");
        }
    }
}

void child_work(int fd, int R)
{
    srand(getpid());
    char c = 'a' + rand() % ('z' - 'a');
    if (write(R, &c, 1) < 0)
        ERR("write to R");
}

void parent_work(int n, int *fds[2], int R){
    char c;
    int ret;
    srand(getpid());
    while((ret = read(R, &c, 1)) == 1){
        printf("%c", c);
    }
    if(ret < 0){
        ERR("read from R");
    }
    printf("\n");
}

int main(int argc, char **argv){
    int n;
    int R[2];
    if (argc != 2){
        usage(argv[0]);
    }
    n = atoi(argv[1]);
    if (n <= 0 || n > 10){
        usage(argv[0]);
    }
    int fds[n][2]; // pipes for each child
    if(pipe(R) < 0){
        ERR("pipe");
    }
    if(sethandler(sigchld_handler, SIGCHLD)){
        ERR("sethandler");
    }
    for(int i = 0; i < n; i++){
        if(pipe(fds[i]) < 0){
            ERR("pipe");
        }
        pid_t pid = fork();
        switch (pid)
        {
        case -1:
            ERR("fork");
            break;
        case 0:
            if(close(fds[i][1]) < 0){
                ERR("close");
            }
            child_work(fds[i][0], R[1]);
            if(close(fds[i][0]) < 0){
                ERR("close");
            }
            if(close(R[1]) < 0){
                ERR("close");
            }
            break;
        }
    }
    parent_work(n, fds, R[0]);
    if(close(R[0]) < 0){
        ERR("close");
    }
    return 0;
}