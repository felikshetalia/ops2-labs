#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sigchld_handler(int sig, siginfo_t *s, void *p)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
        
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n k p l\n", name);
    fprintf(stderr, "100 > n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;        // reasonable default
    attr.mq_msgsize = 64;       // big enough for your string messages



    // server will take a and b and perform the operation
    mqd_t PID_s, PID_d, PID_m, client;
    if(argc != 4){
        printf("Please enter the name for the client PID and 2 integers\n");
        exit(1);
    }
    int a,b;
    int client_pid = atoi(argv[1]);
    a = atoi(argv[2]);
    b = atoi(argv[3]);
    if((PID_s = mq_open("/pid_s", O_CREAT | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((PID_d = mq_open("/pid_d", O_CREAT| O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((PID_m = mq_open("/pid_m", O_CREAT| O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((client = mq_open("/client_queue", O_CREAT| O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }

    printf("My name is pid_s [%d]\n", getpid());
    printf("My name is pid_d [%d]\n", getpid());
    printf("My name is pid_m [%d]\n", getpid());
    sleep(1);

    char name[64];
    ssize_t ret;
    sleep(1);
    // wait for the client to write
    if((ret = mq_receive(client, name, sizeof(name), 0))>=0){
        if(strcmp(name,"pid_s") == 0){
            int sum = a + b;
            char sumch[10];
            sprintf(sumch, "%d", sum);
            if(mq_send(PID_s, sumch, strlen(sumch)+1, 0) < 0){
                ERR("mq_send");
            }
        }
        if(strcmp(name,"pid_d") == 0){
            int res = a / b;
            char resch[10];
            sprintf(resch, "%d", res);
            if(mq_send(PID_d, resch, strlen(resch)+1, 0) < 0){
                ERR("mq_send");
            }
        }
        if(strcmp(name,"pid_m") == 0){
            int mod = a % b;
            char modch[10];
            sprintf(modch, "%d", mod);
            if(mq_send(PID_m, modch, strlen(modch)+1, 0) < 0){
                ERR("mq_send");
            }
        }
    }
    if(ret < 0){
        if(errno == EAGAIN){
            printf("EAGAIN returned please handle it\n");
        }
        else
            ERR("mq_receive");
    }



    if(mq_close(PID_s)<0){
        ERR("mq_close");
    }
    if(mq_close(PID_d)<0){
        ERR("mq_close");
    }
    if(mq_close(PID_m)<0){
        ERR("mq_close");
    }
    if(mq_close(client)<0){
        ERR("mq_close");
    }

    return EXIT_SUCCESS;
}


