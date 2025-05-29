#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))
#define FIFO_NAME "./myfifo"

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
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

// each child gets own pipe
// parent writes to big pipe
// children's pipes reads from the big pipe
double probability = 0.2;
void child_work(int child_pipe, int shared_pipe){
    srand(getpid());
    char c = 'a' + rand() % ('z' - 'a');
    if (write(shared_pipe, &c, 1) < 0)
        ERR("write to R");
}

void parent_work(int num_of_children, int **fd,int shared_pipe){
    srand(time(NULL));
    int r = rand() % num_of_children - 1;
    ssize_t ret;
    srand(getpid());
    char c;
    if((ret = read(shared_pipe, &c, 1)) < 0){
        ERR("read from R");
    }
    else{
        printf("Parent read: %c\n", c);
    }
}

int main(int argc, char** argv){
    int n;
    int shared_pipe[2];
    if(argc != 2){
        usage(argv[0]);
    }
    n = atoi(argv[1]);
    if(n <= 0 || n > 10){
        usage(argv[0]);
    }
    int fds[n][2]; // pipes for each child
    if(sethandler(sigchld_handler, SIGCHLD)){
        ERR("Seting parent SIGCHLD:");
    }
    pid_t pid;
    for(int i = 0; i < n; i++){
        if(pipe(fds[i]) < 0){
            ERR("pipe");
        }
    }
    if(pipe(shared_pipe) < 0){
        ERR("pipe");
    }
    for(int i = 0; i < n; i++){
        pid = fork();
        switch (pid)
        {
        case -1:
            ERR("fork");
        case 0:
            close(fds[i][1]);
            printf("Child %d PID: %d\n", i + 1, getpid());
            child_work(fds[i][0], shared_pipe[1]);
            close(fds[i][0]);
            break;
        }
        parent_work(n, fds[i][1],shared_pipe[0]);
        close(shared_pipe[1]);
        while(wait(NULL) > 0);
    }
    

    return 0;
}