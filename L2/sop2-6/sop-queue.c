#define _GNU_SOURCE
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define MAX_MSG_COUNT 4
#define MAX_ITEMS 8
#define MIN_ITEMS 2
#define SHOP_QUEUE_NAME "/shop"
#define MSG_SIZE 128
#define TIMEOUT 2
#define CLIENT_COUNT 8
#define OPEN_FOR 8
#define START_TIME 8
#define MAX_AMOUNT 16

static const char* const UNITS[] = {"kg", "l", "dkg", "g"};
static const char* const PRODUCTS[] = {"mięsa", "śledzi", "octu", "wódki stołowej", "żelatyny"};

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

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void shop_handler(int sig, siginfo_t *info, void *p){
    mqd_t *shop_q;
    unsigned msg_prio;
    shop_q = (mqd_t *)info->si_value.sival_ptr;

    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGRTMIN;
    notif.sigev_value.sival_ptr = shop_q;
    if (mq_notify(*shop_q, &notif) < 0)
        ERR("mq_notify");

    for (;;)
    {
        char order[MSG_SIZE];
        unsigned int prio;
        if(TEMP_FAILURE_RETRY(mq_receive(*shop_q, order, MSG_SIZE, &prio)) < 0){
            if(errno == EAGAIN || errno == EBADF){
                break;
            }
            else{
                ERR("mq_receive");
            }
        }
        if(prio != 0){
            printf("Please go to the end of the line!\n");
            
        }
        if(prio == 0){
            switch(rand() % 3){
                case 0:
                    printf("Come back tomorrow.\n");
                    break;
                case 1:
                    printf("Out of stock.\n");
                    break;
                case 2:
                    printf("There is an item in the packing zone that shouldn’t be there.\n");
                    break;
                default:
                    break;
            }
        }
        msleep(100);
    }
}

void checkout_work(){
    srand(getpid());
    mqd_t shop_q;
    if((shop_q = mq_open(SHOP_QUEUE_NAME, O_RDONLY | O_NONBLOCK)) < 0){
        ERR("mq_open");
    }
    sethandler(shop_handler, SIGRTMIN);
    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGRTMIN;
    notif.sigev_value.sival_ptr = &shop_q;
    if (mq_notify(shop_q, &notif) < 0)
        ERR("mq_notify");
    int probability = rand() % 4;
    if(probability == 0){
        printf("Closed today\n");
        if(mq_close(shop_q) < 0){
            ERR("mq_close");
        }
        exit(0);
    }
    printf("OPEN today\n");
    for(int i = 0; i < OPEN_FOR; i++){
        // every hour
        printf("%d:00\n", START_TIME + i);
        while(1){
            char order[MSG_SIZE];
            unsigned int prio;
            if(mq_receive(shop_q, order, MSG_SIZE, &prio) < 0){
                if(errno == EAGAIN){
                    break;
                }
                else{
                    ERR("mq_receive");
                }
            }
            if(prio != 0){
                printf("Please go to the end of the line!\n");
            }
            if(prio == 0){
                switch(rand() % 3){
                    case 0:
                        printf("Come back tomorrow.\n");
                        break;
                    case 1:
                        printf("Out of stock.\n");
                        break;
                    case 2:
                        printf("There is an item in the packing zone that shouldn’t be there.\n");
                        break;
                    default:
                        break;
                }
            }
            msleep(200);
        }
        
        msleep(200);
    }
    if(mq_close(shop_q) < 0){
        ERR("mq_close");
    }
    msleep(1000);
}

void client_work(){
    mqd_t shop_q;
    if((shop_q = mq_open(SHOP_QUEUE_NAME, O_WRONLY)) < 0){
        ERR("mq_open");
    }
    srand(getpid());
    int n = rand() % (MAX_ITEMS - MIN_ITEMS + 1) + MIN_ITEMS;
    for(int i = 0; i < n; i++){
        int u = rand() % sizeof(UNITS) / sizeof(char*);
        int p = rand() % sizeof(PRODUCTS) / sizeof(char*);
        int a = rand() % MAX_AMOUNT + 1;
        unsigned int P = rand() % 2;
        
        char order[MSG_SIZE];
        snprintf(order, MSG_SIZE, "%d%s %s", a, UNITS[u], PRODUCTS[p]);
        struct timespec time;
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec++;
        if(mq_timedsend(shop_q, order, MSG_SIZE, P, &time) < 0){
            if(errno == ETIMEDOUT){
                printf("[%d] I will never come here…\n", getpid());
                break;
            }
            else ERR("mq_timedsend");
        }
    }
    if(mq_close(shop_q) < 0){
        ERR("mq_close");
    }
}

void create_children(){
    srand(getpid());
    for(int i = 0; i < CLIENT_COUNT+1; i++){
        pid_t pid = fork();
        if(pid < 0) ERR("fork");
        if(pid == 0 && i > 0){
            // client 
            //printf("[%d] I will never come here…\n", getpid());
            client_work();
            exit(0);
        }
        if(pid == 0 && i == 0){
            // check out 
            //msleep(200);
            checkout_work();
            printf("Store closing...\n");
            exit(0);
        }
        if(pid > 0 && i==0){
            // parent
            msleep(100);
        }
    }
}
int main(void)
{
    mq_unlink(SHOP_QUEUE_NAME);
    mqd_t shop_q;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MSG_COUNT;
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_curmsgs = 0;
    if((shop_q = mq_open(SHOP_QUEUE_NAME, O_CREAT | O_EXCL | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if(mq_close(shop_q) < 0){
        ERR("mq_close");
    }
    create_children();
    while(wait(NULL) > 0);
    mq_unlink(SHOP_QUEUE_NAME);
    return EXIT_SUCCESS;
}


// 2 queues
// 1 -struct
// the other one string
// one q for unprocessed items
// processed - string
// instead of threads we have signals

// trick: one oricess sends unprcess stuff and in a loop, nonblovking
// child process will have to know if the otr process finished

// fork and pass the same fd to the child

// remeber abt attr passing
// we can add sleeps anywhere
// registre notifications in main thread (in the beginnign)

// no need to clean up the queue after notif

// Note: How to send struct data?

// struct timespec t;
// t = {...}
// mq_send(queuefd, (char*)&t, sizeof(struct timespec), prio);

// Note: How to send a byte?
// struct timespec t;
// mq_receive(fd, (char*)&t, sizeof(t));