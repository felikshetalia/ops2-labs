
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
void sigusr1_handler(int sig, siginfo_t *s, void *p){
    mqd_t *fd = (mqd_t *)s->si_value.sival_ptr;
    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGRTMIN;
    notif.sigev_value.sival_ptr = fd;
    if (mq_notify(*fd, &notif) < 0)
        ERR("mq_notify");
    int clients = 0;
    while(1){
        if(clients == MAX_CLIENTS) break;
        char buf[MSG_SIZE];
        unsigned int p;
        if(mq_receive(*fd, buf,MSG_SIZE,&p) < 0){
            if(errno == EAGAIN )
            {
                break;

            }
            else
                ERR("mq_receive");
        }
        if(p == 0){
            printf("[%d] Client %s has connected!\n",p, buf);
            clients++;
        }
        if(p == 1){
            printf("[%d] Client %s has disconnected!\n",p, buf);
            clients--;
        }
        if(p==2){
            printf("[%d] Client %s:\n",p, buf);
        }
    }

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
    sethandler(sigusr1_handler, SIGUSR1);
    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGUSR1;
    notif.sigev_value.sival_ptr = &server_q;
    if (mq_notify(server_q, &notif) < 0)
        ERR("mq_notify");
    while(1){
        if(currentcli == MAX_CLIENTS){
            printf("Server has reached capacity, wait for room");
            continue;
        }
        char buf[MSG_SIZE];
        char client_name[MSG_SIZE];
        unsigned int p;
        if(mq_receive(server_q, buf,MSG_SIZE,&p) < 0){
            if(errno == EAGAIN )
            {
                break;

            }
            else
                ERR("mq_receive");
        }
        if(p == 0){
            snprintf(client_name, MSG_SIZE, "%s", buf);
            printf("[%d] Client %s has connected!\n",p, buf);
            currentcli++;
        }
        if(p == 1){
            printf("[%d] Client %s has disconnected!\n",p, client_name);
            currentcli--;
        }
        if(p==2){
            printf("[%d] Client %s: %s\n",p, client_name, buf);
        }

    }
    free(clients);
    mq_close(server_q);
    mq_unlink(server_name);

    return EXIT_SUCCESS;
}
