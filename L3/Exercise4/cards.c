#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <unistd.h>
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SEMAWHORE "/semmy"
#define SHMMEM "/shared"


void usage(char *name){
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "2<=n<=5 - 5<=m<=10\n");
    exit(EXIT_FAILURE);
}

typedef struct {
    int *board;
    sem_t* childsem;
} SharedData_t;

void print_board(int* board, int n, int m) {
    for(int i = 0; i < n; i++) {
        printf("PLAYER %d --------------\n", i + 1);
        for(int j = 0; j < m; j++) {
            printf("%2d ", board[i * m + j]);
        }
        printf("\n");
    }
}

void child_work(SharedData_t* data){
    srand(getpid());

    // child will open the sem
    char sem_name[20];
    snprintf(sem_name, 20, "sem%d", getpid());
    if((data->childsem = sem_open(sem_name, O_CREAT, 0600, 1)) == SEM_FAILED){
        ERR("sem_open");
    }
    printf("Child has opened the semaphore named %s\n", sem_name);

    if(sem_close(data->childsem) < 0){
        ERR("sem_close");
    }
}

void create_children(int n, SharedData_t* data){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<n;i++){
        pid = fork();
        if(pid == 0){
            //printf("[%d] Child entered\n", getpid());
            child_work(data);
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }
}

int main(int argc, char **argv) {
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    if (argc != 3) {
        usage(argv[0]);
    }
    if(n < 2 || n > 5 || m < 5 || m > 10){
        usage(argv[0]);
    }

    sem_t* sem;
    if((sem = sem_open(SEMAWHORE, O_CREAT, 0600, 1)) == SEM_FAILED){
        ERR("sem_open");
    }
    sem_wait(sem);
    int fd;
    int isInitialized = 0; // false
    if((fd = shm_open(SHMMEM, O_CREAT | O_EXCL | O_RDWR, 0600)) < 0){
        if(errno == EEXIST){
            isInitialized = 1;
            if((fd = shm_open(SHMMEM, O_RDWR, 0600)) < 0){
                sem_post(sem);
                ERR("shm_open");
            }
        }
        else
            ERR("shm_open");
    }
    if(ftruncate(fd, sizeof(SharedData_t)) < 0){
        shm_unlink(SHMMEM);
        sem_post(sem);
        ERR("ftruncate");
    }

    SharedData_t* data = mmap(NULL, sizeof(SharedData_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(data == MAP_FAILED){
        if(isInitialized){
            sem_post(sem);
            ERR("mmap");
        }
        else{
            shm_unlink(SHMMEM);
            sem_post(sem);
            ERR("mmap");
        }
    }

    close(fd);
    sem_post(sem);

    // init the board

    
    for(int i = 0; i < n; i++){
        for(int j = 0; j < m; j++){
            data->board[i * n + j] = i+1;
        }
    }

    for(int i = 0; i < n; i++){
        printf("PLAYER %d --------------\n", n);
        for(int j = 0; j < m; j++){
            printf("%d ", data->board[i * n + j]);
        }
    }
    create_children(n, data);
    while(wait(NULL) > 0);
    munmap(data, sizeof(SharedData_t));
    if(sem_close(sem) < 0){
        ERR("sem_close");
    }
    shm_unlink(SHMMEM);
    
    return 0;
}