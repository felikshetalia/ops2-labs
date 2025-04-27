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
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MSG_SIZE 128
#define UNPROCS_Q "/unprocessed"
#define PROCS_Q "/processed"


#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

typedef struct Content_t{
    int a;
    int b;
}Content_t

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

int main(void)
{
    mq_unlink(UNPROCS_Q);
    mq_unlink(PROCS_Q);
    mqd_t unprocessed, processed;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_curmsgs = 0;
    if((unprocessed = mq_open(UNPROCS_Q, O_CREAT | O_EXCL | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
    if((processed = mq_open(PROCS_Q, O_CREAT | O_EXCL | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }

    if(mq_close(unprocessed) < 0){
        ERR("mq_close");
    }
    if(mq_close(processed) < 0){
        ERR("mq_close");
    }
    mq_unlink(UNPROCS_Q);
    mq_unlink(PROCS_Q);
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