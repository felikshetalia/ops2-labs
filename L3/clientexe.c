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
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SHM_SIZE 1024

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s server_pid\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if (argc != 2)
        usage(argv[0]);

    const int server_pid = atoi(argv[1]);
    if(server_pid == 0){
        usage(argv[0]);
    }
    srand(getpid());
    char shm_name[SHM_SIZE];
    snprintf(shm_name, SHM_SIZE, "%d-board", server_pid);
    int board;
    if((board = shm_open(shm_name, O_RDWR, 0600)) < 0){
        ERR("shm_open");
    }
    if(ftruncate(board, SHM_SIZE) < 0){
        ERR("ftruncate");
    }
    char* memptr = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, board, 0);
    if(memptr == MAP_FAILED){
        ERR("mmap");
    }

    // pthread_mutex_t *mutex = (pthread_mutex_t*)memptr;
    sem_t *sem = (sem_t*)memptr;
    if (sem_init(sem, 1, 1) == -1) {
        ERR("sem_init");
    }
    char *board_shared = (memptr + sizeof(sem_t));
    char *board_priv = (memptr + sizeof(sem_t)) + 1;
    // server's own board
    const int N = board_shared[0];

    int score = 0;
    while(1){
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
        const int D = rand() % 9 + 1;
        if(D==1){
            printf("Oops...\n");
            exit(0);
        }

        int x,y;
        x = rand()%(N-1) + 1;
        y = rand()%(N-1) + 1;
        printf("Trying to search field (%d,%d)\n", x, y);
        const int P = board_priv[N * y + x];
        if(P==0){
            printf("GAME OVER: score %d\n", score);
            //pthread_mutex_unlock(mutex);
            sem_post(sem);
            break;
        }
        else{
            printf("found %d points\n", P);
            score += P;
            board_priv[N * y + x] = 0;
        }
        //pthread_mutex_unlock(mutex);
        sem_post(sem);
        struct timespec t = {1, 0};
        nanosleep(&t, &t);
    }
    munmap(memptr, SHM_SIZE);
    sem_destroy(sem);
    return EXIT_SUCCESS;
}