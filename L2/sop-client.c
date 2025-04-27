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
    sleep(2); // let server create the queues first
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;        // reasonable default
    attr.mq_msgsize = 64;       // big enough for your string messages

    if(argc != 2){
        printf("Please enter the name for the server queue\n");
        exit(1);
    }
    //unlink("/client_queue");
    mqd_t client_queue, PID_s, PID_d, PID_m;
    if((client_queue = mq_open("/client_queue", O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((PID_s = mq_open("/pid_s",  O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((PID_d = mq_open("/pid_d",  O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((PID_m = mq_open("/pid_m",  O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    //printf("And I am the client %s [%d]\n", argv[1], getpid());
    sleep(1);

    ssize_t ret;
    char res[64];
    if(mq_send(client_queue, argv[1], sizeof(char*), 0)){
        ERR("mq_send");
    }
    else{
        printf("REquest sent to server\n");
    }
    if(strcmp(argv[1], "pid_s") == 0){
        if((ret = mq_receive(PID_s, res, sizeof(res), 0))>=0){
            printf("Result of a and b is = %s\n", res);
        }
        else{
            if(errno == EAGAIN){
                printf("EAGAIN returned please handle it\n");
            }
            else
                ERR("mq_receive");
        }
    }
    if(strcmp(argv[1], "pid_d") == 0){
        if((ret = mq_receive(PID_d, res, sizeof(res), 0))>=0){
            printf("Result of a and b is = %s\n", res);
        }
        else{
            if(errno == EAGAIN){
                printf("EAGAIN returned please handle it\n");
            }
            else
                ERR("mq_receive");
        }
    }
    if(strcmp(argv[1], "pid_m") == 0){
        if((ret = mq_receive(PID_m, res, sizeof(res), 0))>=0){
            printf("Result of a and b is = %s\n", res);
        }
        else{
            if(errno == EAGAIN){
                printf("EAGAIN returned please handle it\n");
            }
            else
                ERR("mq_receive");
        }
    }
    

    if(mq_close(client_queue)<0){
        ERR("mq_close");
    }
    if(mq_unlink("/client_queue")<0){
        ERR("mq_unlink");
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
    if(mq_unlink("/pid_s")<0){
        ERR("mq_unlink");
    }
    if(mq_unlink("/pid_d")<0){
        ERR("mq_unlink");
    }
    if(mq_unlink("/pid_m")<0){
        ERR("mq_unlink");
    }
    return EXIT_SUCCESS;
}
