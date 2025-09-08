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

#define PARENT_QUEUE_NAME "/parent"
#define WORK_QUEUE_NAME "/work"
#define CHILD_COUNT 4
#define MAX_MSG_COUNT 4
#define MAX_RAND_INT 128
#define ROUNDS 512
#define SLEEP_TIME 20
#define FLOAT_ACCURACY RAND_MAX
#define PARENT_MSG_SIZE 1
#define WORK_MSG_SIZE (2 * sizeof(float))

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

float rand_float()
{
    int rand_int = rand() % FLOAT_ACCURACY + 1;
    float rand_float = (float)rand_int / (float)FLOAT_ACCURACY;

    return rand_float * 2.0f - 1.0f;
}
void mq_handler(int sig, siginfo_t *info, void *p);

void msleep(int millisec)
{
    struct timespec tt;
    tt.tv_sec = millisec / 1000;
    tt.tv_nsec = (millisec % 1000) * 1000000;
    while (nanosleep(&tt, &tt) == -1)
    {
    }
}

typedef struct Point_t{
    float x;
    float y;
} Point_t;

int checkInDisc(Point_t pt){
    if(pt.x * pt.x + pt.y * pt.y <= 1){
        return 1;
    }
    return 0;
}

void parent_work(mqd_t workQueue){
    Point_t points[ROUNDS];

    for(int i = 0; i < ROUNDS; i++){
        points[i].x = rand_float();
        points[i].y = rand_float();

        if(mq_send(workQueue, (const char*)&points[i], sizeof(Point_t), 0) < 0)
            ERR("mq_send");
    }

}

void child_work(mqd_t parentQueue, mqd_t workQueue){
    if(mq_close(parentQueue) < 0)
        ERR("mq_close");

    if((parentQueue = mq_open(PARENT_QUEUE_NAME, O_RDWR, 0600)) == (mqd_t)-1)
        ERR("mq_open");

    ssize_t ret;
    Point_t pt;
    while(1){
        if((ret = mq_receive(workQueue, (char*)&pt, sizeof(Point_t), 0)) < 0)
            ERR("mq_receive");
        msleep(SLEEP_TIME);
        printf("Point (x,y): (%.2f, %2.f)\n", pt.x,pt.y);
        if(checkInDisc(pt)){
            printf("Point (%.2f, %2.f) list in the disc!\n", pt.x,pt.y);
        }
    }
}

void create_children(mqd_t parentQueue, mqd_t workQueue){
    srand(getpid());
    for(int i = 0; i < CHILD_COUNT; i++){
        pid_t pid = fork();
        if(pid < 0) ERR("fork");
        if(pid == 0){
            // client 
            printf("[%d] Exiting…\n", getpid());
            child_work(parentQueue, workQueue);
            exit(0);
        }
        if(pid > 0){
            // parent
            parent_work(workQueue);
            msleep(100);
        }
    }
}


int main(void)
{
    mqd_t parentQueue, workerQueue;

    struct mq_attr parentAttr = {0};
    parentAttr.mq_maxmsg = MAX_MSG_COUNT;
    parentAttr.mq_msgsize = PARENT_MSG_SIZE;

    struct mq_attr workerAttr = {0};
    workerAttr.mq_maxmsg = MAX_MSG_COUNT;
    workerAttr.mq_msgsize = WORK_MSG_SIZE;

    if((parentQueue = mq_open(PARENT_QUEUE_NAME, O_CREAT | O_RDWR | O_NONBLOCK, 0600, &parentAttr)) == (mqd_t)-1)
        ERR("mq_open");
    if((workerQueue = mq_open(WORK_QUEUE_NAME, O_CREAT | O_RDWR, 0600, &workerAttr)) == (mqd_t)-1)
        ERR("mq_open");

    if(mq_setattr(parentQueue, &parentAttr, NULL) < 0)
        ERR("mq_setattr");
    if(mq_setattr(workerQueue, &workerAttr, NULL) < 0)
        ERR("mq_setattr");


    // main work
    create_children(parentQueue, workerQueue);
    while(wait(NULL) > 0);

    // cleanup

    if(mq_close(parentQueue) < 0)
        ERR("mq_close");
    if(mq_close(workerQueue) < 0)
        ERR("mq_close");
    if(mq_unlink(PARENT_QUEUE_NAME) < 0)
        ERR("mq_close");
    if(mq_unlink(WORK_QUEUE_NAME) < 0)
        ERR("mq_close");
    return EXIT_SUCCESS;
}

// https://sop.mini.pw.edu.pl/en/sop2/lab/l3/#solution-1
// https://github.com/michau-s/OPS2-L2-Consultations/blob/master/sop2-6/sop-queue.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L2/exercise2.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L2/sop2-6/sop-queue.c
// felikshetalia is me