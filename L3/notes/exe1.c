#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSG_LEN 128
#define SEMAWHORE "/something"
#define SHM_NAME "/pussay"

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


void usage(char* name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "10000 >= n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

typedef struct SharedData_t{
    int numChildren;
    int counter;
    int finished_children;
    pthread_mutex_t mx;
    pthread_cond_t op;
    int ready; //flag
}SharedData_t;

void mutex_lock_handler(pthread_mutex_t* mutex){
    int error;
    if ((error = pthread_mutex_lock(mutex)) != 0)
    {
        if (error == EOWNERDEAD)
        {
            pthread_mutex_consistent(mutex);
        }
        else
        {
            ERR("pthread_mutex_lock");
        }
    }
}

pthread_mutexattr_t init_mtx(){
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&mutexattr, PTHREAD_MUTEX_ROBUST);
    return mutexattr;
}

void operation_bruh(SharedData_t* dataobj){
    //SharedData_t* dataobj = (SharedData_t*)arg;
    int n = dataobj->numChildren;
    for(int i=0;i<n;i++){
        mutex_lock_handler(&dataobj->mx);
        dataobj->counter = dataobj->counter / n;
        pthread_mutex_unlock(&dataobj->mx);
        printf("Operation yields = %d\n", dataobj->counter);
    }
}

void child_work(int n, SharedData_t* dataobj){
    
    mutex_lock_handler(&dataobj->mx);
    dataobj->counter += getpid();
    pthread_mutex_unlock(&dataobj->mx);

    printf("Child with pid [%d]: I've summed %d\n", getpid(), dataobj->counter);

    mutex_lock_handler(&dataobj->mx);
    dataobj->finished_children++;
    if (dataobj->finished_children == dataobj->numChildren) {
        dataobj->ready = 1;
        pthread_cond_signal(&dataobj->op); // only the last child signals
    }
    pthread_mutex_unlock(&dataobj->mx);
    exit(0);
}

void pop_children(int n, SharedData_t *dataobj){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<n;i++){
        pid = fork();
        if(pid == 0){
            printf("[%d] Child entered\n", getpid());
            child_work(n, dataobj);
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }
}

int main(int argc, char** argv)
{
    if(argc !=2) usage(argv[0]);
    int n = atoi(argv[1]);

    sem_t *sem;
    // if(sem_init(&sem, 1, 0) < 0){
    //     ERR("sem_init");
    // }
    if((sem = sem_open(SEMAWHORE, O_CREAT, 0666, 1)) == SEM_FAILED){
        ERR("sem_open");
    }
    sem_wait(sem);
    int fd;
    if((fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR | O_TRUNC, 0600)) < 0){
        if(errno == EEXIST){
            if((fd = shm_open(SHM_NAME, O_RDWR, 0600)) < 0){
                sem_post(sem);
                ERR("shm_open");
            }
        }
        else
            ERR("shm_open");
    }
    if(ftruncate(fd, sizeof(SharedData_t)) < 0){
        sem_post(sem);
        shm_unlink(SHM_NAME);
        ERR("ftruncate");
    }
    
    SharedData_t* dataobj;
    if(MAP_FAILED == (dataobj = mmap(NULL, sizeof(SharedData_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0))){
        shm_unlink(SHM_NAME);
        sem_post(sem);
        ERR("mmap");
    }
    close(fd);
    sem_post(sem);
    pthread_mutexattr_t attr = init_mtx();
    pthread_mutex_init(&dataobj->mx, &attr);
    dataobj->counter = 0;
    dataobj->numChildren = n;
    // 
    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
    int ret;
    if((ret = pthread_cond_init(&dataobj->op, &condattr)) != 0){
        ERR("cond_init");
    }
    pop_children(n, dataobj);
    //printf("Final result: ");
    mutex_lock_handler(&dataobj->mx);
    while (dataobj->ready == 0) {
        pthread_cond_wait(&dataobj->op, &dataobj->mx);
    }
    pthread_mutex_unlock(&dataobj->mx);
    operation_bruh(dataobj);

    while(wait(NULL) > 0);
    pthread_mutex_destroy(&dataobj->mx);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_destroy(&dataobj->op);
    munmap(dataobj, sizeof(SharedData_t));

    sem_close(sem);
    return EXIT_SUCCESS;
}