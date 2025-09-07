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
typedef struct Point_t
{
    float x;
    float y;
} Point_t;

Point_t *create_points()
{
    srand(getpid());
    Point_t *points = calloc(ROUNDS, sizeof(Point_t));
    for (int i = 0; i < ROUNDS; i++)
    {
        points[i].x = rand_float();
        points[i].y = rand_float();
    }
    return points;
}

int check_in_circle(Point_t p)
{
    float dist = p.x * p.x + p.y * p.y;
    if (dist <= 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

float calculate_pi(int msg_count);

void mq_handler(int sig, siginfo_t *info, void *p)
{
    // calculate pi
    mqd_t *parentq;
    unsigned msg_prio;
    int msg_count = 0;
    int circle_count = 0;
    float pi = 0.0;
    parentq = (mqd_t *)info->si_value.sival_ptr;

    // struct mq_attr attr;
    // mq_getattr(*parentq, &attr);

    static struct sigevent notif;
    notif.sigev_notify = SIGEV_SIGNAL;
    notif.sigev_signo = SIGRTMIN;
    notif.sigev_value.sival_ptr = parentq;
    if (mq_notify(*parentq, &notif) < 0)
        ERR("mq_notify");

    for (;;)
    {
        Point_t pt;
        ssize_t ret;
        if ((ret = mq_receive(*parentq, (char *)&pt, sizeof(pt), &msg_prio)) < 0)
        {
            if (errno == EAGAIN)
                break;
            else
                ERR("mq_receive");
        }
        if (ret >= 0)
        {
            msg_count++;
            if (msg_count == MAX_MSG_COUNT)
                break;
            printf("Coordinate: X: %lf, Y: %lf\n", pt.x, pt.y);
            if (check_in_circle(pt) == 1)
            {
                printf("Coordinate (%lf, %lf) lies on (0,0)\n", pt.x, pt.y);
                circle_count++;
            }
        }
    }
}

void parent_work(mqd_t workq, mqd_t parentq)
{
    srand(getpid());
    // static struct sigevent notif;
    // notif.sigev_notify = SIGEV_SIGNAL;
    // notif.sigev_signo = SIGRTMIN;
    // notif.sigev_value.sival_ptr = &parentq;
    // if (mq_notify(parentq, &notif) < 0)
    //     ERR("mq_notify");
    // Point_t* points = calloc(ROUNDS, sizeof(Point_t));
    Point_t points[ROUNDS];
    for (int i = 0; i < ROUNDS; i++)
    {
        points[i].x = rand_float();
        points[i].y = rand_float();
        // printf("Coordinate: X: %lf, Y: %lf\n", points[i].x, points[i].y);
        if (mq_send(workq, (char *)&points[i], sizeof(points[i]), 0) < 0)
        {
            if (errno == EAGAIN)
                printf("Queue is full\n");
            else
                ERR("mq_send");
        }
    }
}

void child_work(mqd_t parentq, mqd_t workq)
{
    if ((parentq = mq_open(PARENT_QUEUE_NAME, O_RDONLY)) == (mqd_t)-1)
        ERR("mq_open parent");

    while (1)
    {
        Point_t pt;
        unsigned int pr;
        if (mq_receive(workq, (char *)&pt, sizeof(pt), &pr) < 0)
        {
            if (errno == EAGAIN)
            {
                printf("EAGAIN\n");
                break;
            }
            else
                ERR("mq_receive");
        }
        else
        {
            printf("Coordinate: X: %lf, Y: %lf\n", pt.x, pt.y);
            if (check_in_circle(pt) == 1)
            {
                printf("Coordinate (%lf, %lf) lies on (0,0)\n", pt.x, pt.y);
                if (mq_send(parentq, (char *)&pt, sizeof(pt), 1) < 0)
                {
                    if (errno == EAGAIN)
                    {
                        printf("EAGAIN\n");
                        break;
                    }
                    else
                        ERR("mq_send");
                }
            }
            else
            {
                if (mq_send(parentq, (char *)&pt, sizeof(pt), 0) < 0)
                {
                    if (errno == EAGAIN)
                    {
                        printf("EAGAIN\n");
                        break;
                    }
                    else
                        ERR("mq_send");
                }
            }
        }
        msleep(SLEEP_TIME);
    }
    if (mq_close(parentq) < 0)
    {
        ERR("mq_close");
    }
}

void create_children(mqd_t parentq, mqd_t workq)
{
    for (int i = 0; i < CHILD_COUNT; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // child
            // printf("[%d] Starting...\n", getpid());
            child_work(parentq, workq);
            printf("[%d] Exiting...\n", getpid());
            exit(0);
        }
        if (pid < 0)
        {
            ERR("fork");
        }
        if (pid > 0)
        {
            parent_work(workq, parentq);
            // fine
        }
    }
}

int main(void)
{
    srand(getpid());

    mqd_t parent_q;
    mqd_t work_q;

    struct mq_attr attr_parent;
    attr_parent.mq_flags = 0;
    attr_parent.mq_maxmsg = MAX_MSG_COUNT;
    attr_parent.mq_msgsize = 1;
    attr_parent.mq_curmsgs = 0;

    struct mq_attr attr_work;
    attr_work.mq_flags = 0;
    attr_work.mq_maxmsg = MAX_MSG_COUNT;
    attr_work.mq_msgsize = 2 * sizeof(float);
    attr_work.mq_curmsgs = 0;

    if ((parent_q = mq_open(PARENT_QUEUE_NAME, O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr_parent)) == (mqd_t)-1)
        ERR("mq_open parent");
    if ((work_q = mq_open(WORK_QUEUE_NAME, O_RDWR | O_CREAT, 0600, &attr_work)) == (mqd_t)-1)
        ERR("mq_open work");

    if (mq_close(parent_q) < 0)
    {
        ERR("mq_close");
    }
    create_children(parent_q, work_q);

    while (wait(NULL) > 0)
        ;
    mq_close(parent_q);
    mq_close(work_q);

    mq_unlink(PARENT_QUEUE_NAME);
    mq_unlink(WORK_QUEUE_NAME);
    return EXIT_SUCCESS;
}

// https://sop.mini.pw.edu.pl/en/sop2/lab/l3/#solution-1
// https://github.com/michau-s/OPS2-L2-Consultations/blob/master/sop2-6/sop-queue.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L2/exercise2.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L2/sop2-6/sop-queue.c
// felikshetalia is me