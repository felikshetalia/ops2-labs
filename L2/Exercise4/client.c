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
    char server_name[MAXNAME];
    snprintf(server_name, MAXNAME, "/chat_%s", argv[1]);
    mqd_t server_q;
    if((server_q = mq_open(server_name, O_WRONLY)) < 0){
        ERR("mq_open");
    }
    char msg[50];
    snprintf(msg, 50, "Your name is %s", server_name);
    if(mq_send(server_q,msg,strlen(msg)+1, 0) < 0){
        ERR("mq_send");
    }
    else
        printf("Message sent successfully!\n");

    mq_close(server_q);
    return EXIT_SUCCESS;
}


