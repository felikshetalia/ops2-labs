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


int main(void)
{
    
    return EXIT_SUCCESS;
}

// https://sop.mini.pw.edu.pl/en/sop2/lab/l3/#solution-1
// https://github.com/michau-s/OPS2-L2-Consultations/blob/master/sop2-6/sop-queue.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L2/exercise2.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L2/sop2-6/sop-queue.c
// felikshetalia is me