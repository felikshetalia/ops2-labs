#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MSG_SIZE 128
#define MAX_CLIENTS 8
#define UNPROCS_Q "/unprocessed"
#define PROCS_Q "/processed"


#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


void msleep(int millisec)
{
    struct timespec tt;
    tt.tv_sec = millisec / 1000;
    tt.tv_nsec = (millisec % 1000) * 1000000;
    while (nanosleep(&tt, &tt) == -1)
    {
    }
}
typedef struct Client_t{
    char name[MSG_SIZE];
    mqd_t queue;
    int isActive;
}Client_t;
void usage(char *name)
{
		fprintf(stderr, "USAGE: %s fifo_file\n", name);
			exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

int main(int argc, char** argv)
{
    if(argc!=2){
        usage(argv[0]);
    }

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_curmsgs = 0;

    char server_name[MSG_SIZE];
    snprintf(server_name, MSG_SIZE,"/chat_%s", argv[1]);
    mqd_t server_q;
    if((server_q = mq_open(server_name, O_CREAT | O_RDWR, 0666, &attr)) < 0){
        ERR("mq_open");
    }
    Client_t* clients;
    if ((clients = calloc(MAX_CLIENTS, sizeof(Client_t))) == NULL)
        ERR("calloc");
    int currentcli = 0;
    while(1){
        if(currentcli == MAX_CLIENTS) break;
        char buf[MSG_SIZE];
        unsigned int p;
        if(mq_receive(server_q, buf,MSG_SIZE,&p) < 0){
            if(errno == EAGAIN )
            {
                break;

            }
            else
                ERR("mq_receive");
        }
        else{
            printf("[%d] Client %s has connected!\n",p, buf);
        }
        currentcli++;

    }
    free(clients);
    mq_close(server_q);
    mq_unlink(server_name);

    return EXIT_SUCCESS;
}