#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SHM_SIZE 1024

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s N\n", name);
    fprintf(stderr, "3 < N <= 30 - board size\n");
    exit(EXIT_FAILURE);
}
typedef struct
{
    int running;
    //pthread_mutex_t mutex;
    sem_t *semaphore;
    sigset_t old_mask, new_mask;
} sighandling_args_t;

void* sighandling(void* args)
{
    sighandling_args_t* sighandling_args = (sighandling_args_t*)args;
    int signo;
    if (sigwait(&sighandling_args->new_mask, &signo))
        ERR("sigwait failed.");
    if (signo != SIGINT)
    {
        ERR("unexpected signal");
    }

    //pthread_mutex_lock(&sighandling_args->mutex);
    sem_wait(sighandling_args->semaphore);
    sighandling_args->running = 0;
    //pthread_mutex_unlock(&sighandling_args->mutex);
    sem_post(sighandling_args->semaphore);
    return NULL;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
        usage(argv[0]);

    const int N = atoi(argv[1]);
    if (N < 3 || N >= 100)
        usage(argv[0]);

    pid_t pid = getpid();
    srand(getpid());

    printf("My PID is %d\n", pid);
    char shm_name[SHM_SIZE];
    snprintf(shm_name, SHM_SIZE, "%d-board", pid);
    int board;
    if((board = shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0600)) < 0){
        ERR("shm_open");
    }
    if(ftruncate(board, SHM_SIZE) < 0){
        ERR("ftruncate");
    }
    char* memptr = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, board, 0);
    if(memptr == MAP_FAILED){
        ERR("mmap");
    }
    sem_t *sem = (sem_t*)memptr;
    if (sem_init(sem, 1, 1) == -1) {
        ERR("sem_init");
    }
    //pthread_mutex_t *mutex = (pthread_mutex_t*)memptr;
    char *board_shared = (memptr + sizeof(sem_t));
    char *board_priv = (memptr + sizeof(sem_t)) + 1;
    // server's own board
    board_shared[0] = N;

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            board_priv[i * N + j] = 1 + rand() % 9;
        }
    }
    // pthread_mutexattr_t attr;
    // pthread_mutexattr_init(&attr);
    // pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    // pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    // pthread_mutex_init(mutex, &attr);

    if(sem_init(sem, 1, 1)<0) ERR("sem_init");
    sighandling_args_t sighandling_args = {1, sem};

    sigemptyset(&sighandling_args.new_mask);
    sigaddset(&sighandling_args.new_mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &sighandling_args.new_mask, &sighandling_args.old_mask))
        ERR("SIG_BLOCK error");

    pthread_t sighandler;
    pthread_create(&sighandler, NULL, sighandling, &sighandling_args);

    while(1)
    {
        //pthread_mutex_lock(&sighandling_args.mutex);
        sem_wait(sighandling_args.semaphore);
        if(!sighandling_args.running)
            break;
        //pthread_mutex_unlock(&sighandling_args.mutex);
        sem_post(sighandling_args.semaphore);
        
        // int err;
        // if((err = pthread_mutex_lock(mutex)) != 0){
        //     if(err == EOWNERDEAD){
        //         pthread_mutex_consistent(mutex);
        //     }
        //     else
        //     {
        //         ERR("pthread_mutex_lock");
        //     }
        // }
        sem_wait(sem);
        // do board thing
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                printf("%d", board_priv[i * N + j]);
            }
            putchar('\n');
        }
        putchar('\n');
        //pthread_mutex_unlock(mutex);
        sem_post(sem);
        struct timespec t = {3, 0};
        nanosleep(&t, &t);
    }
    pthread_join(sighandler, NULL);

    // pthread_mutexattr_destroy(&attr);
    // pthread_mutex_destroy(mutex);
    sem_destroy(sem);
    munmap(memptr, SHM_SIZE);
    shm_unlink(shm_name);
    return EXIT_SUCCESS;
}