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

#define QUEUE_NAME "/workload"

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t stop_requested = 0;

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
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
        
    }
}

void sigint_handler(int sig){
    stop_requested = 1;
}

typedef struct task_t{
    float v1;
    float v2;
    pid_t worker_pid;
    int flag; // if 1 = working, if 0 = stop
}task_t;

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n k p l\n", name);
    fprintf(stderr, "100 > n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}
void msleep(int millisec)
{
    struct timespec tt;
    tt.tv_sec = millisec / 1000;
    tt.tv_nsec = (millisec % 1000) * 1000000;
    while (nanosleep(&tt, &tt) == -1)
    {
    }
}

task_t spawn(){
    task_t t;
    t.v1 = ((float)rand() / RAND_MAX) * 100.0f;
    t.v2 = ((float)rand() / RAND_MAX) * 100.0f;
    t.worker_pid = 0; // not assigned
    t.flag = 1;
    return t;
}

void child_work(){
    //msleep(rand()%5000 + 200);
    mqd_t queue = mq_open(QUEUE_NAME, O_RDONLY);
    if(queue < 0){
        ERR("mq_open");
    }
    int tasks_done = 0;
    while(tasks_done < 5){
        task_t task;
        ssize_t ret;
        if((ret = mq_receive(queue, (char*)&task, sizeof(task), 0)) < 0){
            if(errno == EAGAIN)
                continue;
            else
                ERR("mq_receive");
        }
        if(task.flag == 0) break; //stop signal
        printf("[%d] Received task [%.2f, %.2f]\n", getpid(), task.v1, task.v2);
        float result = task.v1 + task.v2;
        msleep((rand() % 1501 + 500)); // 500–2000 ms sleep
        printf("[%d] Result [%.2f]\n", getpid(), result);
        printf("[%d] Result sent [%.2f]\n", getpid(), result);
        tasks_done++;
    }
}

void parent_work(int n, int t1, int t2, mqd_t queue){
    // parent sends tasks
    int iter = 5*n;
    while(stop_requested == 0 && iter > 0){
        task_t task = spawn();
        ssize_t ret;
        if((ret = mq_send(queue, (const char*)&task, sizeof(task), 0)) < 0){
            if(errno == EAGAIN)
                printf("Queue is full\n");
            else
                ERR("mq_send");
        }
        else{
            printf("New task queued: [%.2f, %.2f]\n", task.v1, task.v2);
        }
        msleep((rand() % (t2 - t1 + 1)) + t1);
        iter--;
    }
}

void create_children(int n){
    srand(getpid());
    for(int i = 0; i < n; i++){
        pid_t pid = fork();
        if(pid == 0){
            //child
            printf("Worker [%d] ready!\n", getpid());
            child_work();
            printf("Worker [%d] exits!\n", getpid());
            exit(0);

        }
        if(pid < 0) ERR("fork");
    }
}

int main(int argc, char **argv)
{
    if(argc != 4) usage(argv[0]);
    int n = atoi(argv[1]);
    int t1 = atoi(argv[2]);
    int t2 = atoi(argv[3]);
    if(n < 2 || n > 20) usage(argv[0]);
    if(t1 < 100 || t1 > t2) usage(argv[0]);
    if(t2 < t1 || t2 > 5000) usage(argv[0]);
    mq_unlink(QUEUE_NAME);

    mqd_t queue;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(task_t);
    attr.mq_curmsgs = 0;
    if((queue = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }

    sethandler(sigint_handler,SIGINT);
    printf("Server is starting...\n");
    create_children(n);
    parent_work(n,t1,t2, queue);

    while(wait(NULL) > 0);
    printf("All the child processses finished\n");

    mq_close(queue);
    mq_unlink(QUEUE_NAME);
    return EXIT_SUCCESS;
}


