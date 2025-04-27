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

#define PIN "/pin"
#define POUT "/pout"

#define LIFE_SPAN 10
#define MAX_NUM 10

#define MAX_MSG_SIZE 64

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t children_left = 0;

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void mq_handler(int sig, siginfo_t *info, void *p)
{
    mqd_t *pin;
    char bingo;
    unsigned int prio;
    pin = (mqd_t *)info->si_value.sival_ptr;
    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGRTMIN;
    notif.sigev_value.sival_ptr = pin;
    if (mq_notify(*pin, &notif) < 0)
        ERR("mq_notify");
    for(;;){
        ssize_t ret;
        if((ret = mq_receive(*pin, &bingo, 1, &prio)) >= 0){
            if(0 == prio)
                printf("MQ: got timeout from %d.\n", (int)(bingo-'0'));
            else
                printf("MQ: %d is a bingo number!\n", (int)(bingo-'0'));
        }
        if(ret < 0 && errno == EAGAIN)
            break;
        if(ret < 0){
            ERR("mq_receive");
        }
        
    }
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
        children_left--;
    }
}

void child_work(int n, mqd_t pin, mqd_t pout)
{
    srand(getpid());
    int bingo = rand()%MAX_NUM;
    char num;
    char nchar = n + '0';
    char bingo_holder = bingo + '0';
    unsigned int prio;
    if((pout = mq_open(POUT, O_RDONLY, 0600, NULL)) < 0){
        ERR("mq_open");
    }
    if(mq_receive(pout, &num, 1,&prio) < 0){
        ERR("mq_receive");
    }
    else{
        printf("I got %c!\n", num);
    }
    if((num-'0') == bingo){
        printf("Child %d got a bingo!\n", getpid());
        if(mq_send(pin, &bingo_holder, 1, 1) < 0){
            ERR("mq_send");
        }
        // sends to say it has bingo
    }
    else{
        if (TEMP_FAILURE_RETRY(mq_send(pin, &nchar, 1, 0)))
            ERR("mq_send");
    }   
    //sleep(1);
}

void parent_work(mqd_t pout, int n)
{
    srand(getpid());
    // if((pout = mq_open(POUT, O_WRONLY, 0600, NULL)) < 0){
    //     ERR("mq_open");
    // }
    for(int i = 0; i < n; i++){
        int randnum = rand() % 10;
        char c = randnum + '0';
        if(mq_send(pout, &c, 1, 0) < 0){
            ERR("mq_send");
        }
        fflush(stdout);
        sleep(1);
    }
    printf("Parent terminates\n");
}

void fork_children(int n, mqd_t pin, mqd_t pout){
    srand(getpid());
    for(int i = 0; i<n; i++){
        pid_t pid = fork();
       
        switch(pid){
            case -1:
                ERR("fork");
            case 0:
                // child here
                printf("Child %d with pid [%d] has entered, ",i+1, getpid());
                child_work(n,pin,pout);
                //sleep(1);
                exit(0);
                //break;
            default:
                //parent_work(pout);
                //sleep(1);
                //exit(0);
                break;
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
    int n;
    if (argc != 2)
        usage(argv[0]);
    n = atoi(argv[1]);
    if (n <= 0 || n >= 100)
        usage(argv[0]);

    mq_unlink(PIN);
    mq_unlink(POUT);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr)); 
    attr.mq_flags = 0;  
    attr.mq_maxmsg = MAX_NUM;
    attr.mq_msgsize = 1; 

    mqd_t pin, pout;
    if((pin = mq_open(PIN, O_CREAT | O_NONBLOCK | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((pout = mq_open(POUT, O_CREAT | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    sethandler(sigchld_handler, SIGCHLD);
    sethandler(mq_handler, SIGRTMIN);
    //parent_work(pout);
    
    fork_children(n,pin,pout);
    static struct sigevent no;
    no.sigev_notify = SIGEV_SIGNAL;
    no.sigev_signo = SIGRTMIN;
    no.sigev_value.sival_ptr = &pin;
    if(mq_notify(pin, &no) < 0){
        ERR("mq_notify");
    }
    
    parent_work(pout,n);
    while(wait(NULL) > 0);

    mq_close(pin);
    mq_close(pout);
    mq_unlink(PIN);
    mq_unlink(POUT);
    return EXIT_SUCCESS;
}
