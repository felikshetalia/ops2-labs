#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#define SHM_NAME "/some_name"
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), printf("%d\n", errno), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t keepRunning = 1;

typedef struct{
    int64_t pid;
    int64_t card;
}block16_t;

int count_descriptors()
{
    int count = 0;
    DIR* dir;
    struct dirent* entry;
    struct stat stats;
    if ((dir = opendir("/proc/self/fd")) == NULL)
        ERR("opendir");
    char path[PATH_MAX];
    getcwd(path, PATH_MAX);
    chdir("/proc/self/fd");
    do
    {
        errno = 0;
        if ((entry = readdir(dir)) != NULL)
        {
            if (lstat(entry->d_name, &stats))
                ERR("lstat");
            if (!S_ISDIR(stats.st_mode))
                count++;
        }
    } while (entry != NULL);
    if (chdir(path))
        ERR("chdir");
    if (closedir(dir))
        ERR("closedir");
    return count - 1;  // one descriptor for open directory
}

void player_work(int writeEnd, block16_t shmptr){
    srand(getpid());
    // printf("I am a player with the PID [%d] and I'm ready\n", getpid());
    shmptr.pid = getpid();
    int value = rand()%(10 - 5 + 1) + 5;
    shmptr.card = value;

    if(write(writeEnd, &shmptr, sizeof(block16_t)) < 0){
        ERR("write");
    }
}

void server_work(int readEnd){
    ssize_t ret;
    block16_t buf;
    if((ret = read(readEnd, &buf, 16)) < 0){
        ERR("read");
    }
    printf("Got number %ld from player %ld\n", buf.card, buf.pid);
}

void create_players(int N, int shfd){
    block16_t* sharedBlock = (block16_t*)mmap(NULL, sizeof(block16_t) * N, PROT_READ | PROT_WRITE, MAP_SHARED, shfd, 0);
    if(sharedBlock == MAP_FAILED){
        ERR("mmap");
    }
    
    int *sendPipes = calloc(2*N, sizeof(int));
    int *recvPipes = calloc(2*N, sizeof(int));
    if(sendPipes == NULL)
        ERR("calloc");
    if(recvPipes == NULL)
        ERR("calloc");

    // two loops, one for receiving, one for sending

    for(int i = 0; i < N; i++){
        if(pipe(&sendPipes[2*i]) < 0){
            ERR("pipe");
        }
        if(pipe(&recvPipes[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < N; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < N; j++){
                if(i!=j){
                    if(close(sendPipes[2*j]) < 0){
                        ERR("close");
                    }
                    if(close(sendPipes[2*j+1]) < 0){
                        ERR("close");
                    }   
                }
                if(close(recvPipes[2*j]) < 0){
                    ERR("close");
                }
                if(close(recvPipes[2*j+1]) < 0){
                    ERR("close");
                }
            }
            player_work(sendPipes[2*i+1], sharedBlock[i]);
            if(close(sendPipes[2*i]) < 0){
                ERR("close");
            }
            if(close(sendPipes[2*i+1]) < 0){
                ERR("close");
            }
            free(sendPipes);
            free(recvPipes);
            munmap(sharedBlock, N * sizeof(block16_t));

            exit(0);
        }
        if(pid < 0)
            ERR("fork");

        server_work(sendPipes[2*i]);
    }

    // cleanup
    for(int i = 0; i < 2*N; i++){
        if(close(sendPipes[i]) < 0){
            ERR("close");
        }
        if(close(recvPipes[i]) < 0){
            ERR("close");
        }
    }
    free(sendPipes);
    free(recvPipes);
    munmap(sharedBlock, N * sizeof(block16_t));
}


int main(int argc, char **argv){
    if(argc != 3){
        printf("2<=N<=5\n5<=M<=10\n");
        exit(0);
    }

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    if(N < 2 || N > 5 || M < 5 || M > 10){
       printf("2<=N<=5\n5<=M<=10\n");
        exit(0); 
    }

    int shfd;
    if(0 > (shfd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600))){
        if(errno == EEXIST){
            if(0 > (shfd = shm_open(SHM_NAME, O_RDWR, 0600))){
                ERR("shm_open");
            }
        }
        else{
            ERR("shm_open");
        }
    }
    else{
        if(0 > ftruncate(shfd, sizeof(block16_t) * N)){
            ERR("ftruncate");
        }
    }

    create_players(N, shfd);
    if(close(shfd) < 0){
        ERR("close");
    }
    while(wait(NULL)>0);
    printf("Opened descriptors: %d\n", count_descriptors());
    shm_unlink(SHM_NAME);
    return 0;
}