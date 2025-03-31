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

#define MAXNAME 30
#define MAXCLIENTS 8
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

typedef struct Client_t{
    char client_name[MAXNAME];
    mqd_t client_queue;
    int activity;
}Client_t;

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n k p l\n", name);
    fprintf(stderr, "100 > n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if(argc!=2){
        usage(argv[0]);
    }

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 100;
    attr.mq_curmsgs = 0;

    char server_name[MAXNAME];
    char client_name[MAXNAME];
    snprintf(server_name, MAXNAME, "/chat_%s", argv[1]);
    mq_unlink(server_name);
    mqd_t server_q, client_q;
    if((server_q = mq_open(server_name, O_CREAT | O_RDWR, 0666, &attr)) < 0){
        ERR("mq_open");
    }
    char buffer[100];
    unsigned int pr;
    if(mq_receive(server_q, client_name, 100, &pr) < 0){
        ERR("mq_receive");
    }
    // else{
    //     //printf("[%d] %s\n", pr, buffer);
    //     printf("Client %s has connected!\n",client_name);
    // }
    if((client_q = mq_open(client_name, O_RDONLY)) < 0){
        ERR("mq_open");
    }
    else{
        printf("Client %s has connected!\n",client_name);
    }

    mq_close(server_q);
    mq_close(client_q);
    if(mq_unlink(server_name) < 0){
        ERR("mq_unlink");
    }
    if(mq_unlink(client_name) < 0){
        ERR("mq_unlink");
    }
    return EXIT_SUCCESS;
}


