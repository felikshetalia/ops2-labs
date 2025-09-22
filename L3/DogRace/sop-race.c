#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define LEADERBOARD_FILENAME "./leaderboard"
#define LEADERBOARD_ENTRY_LEN 32

#define MIN_TRACK_LEN 16
#define MAX_TRACK_LEN 256
#define MIN_DOG_COUNT 2
#define MAX_DOG_COUNT 6

#define MIN_MOVEMENT 1
#define MAX_MOVEMENT 6
#define MIN_SLEEP 250
#define MAX_SLEEP 1500

#define ERR(source)                                     \
    do                                                  \
    {                                                   \
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); \
        perror(source);                                 \
        kill(0, SIGKILL);                               \
        exit(EXIT_FAILURE);                             \
    } while (0)

void usage(char* program_name)
{
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "  %s L N\n", program_name);
    fprintf(stderr, "    L - race track length, %d <= L <= %d\n", MIN_TRACK_LEN, MAX_TRACK_LEN);
    fprintf(stderr, "    N - number of dogs, %d <= N <= %d\n", MIN_DOG_COUNT, MAX_DOG_COUNT);
    exit(EXIT_FAILURE);
}

void msleep(unsigned int milli)
{
    time_t sec = (int)(milli / 1000);
    milli = milli - (sec * 1000);
    struct timespec ts = {0};
    ts.tv_sec = sec;
    ts.tv_nsec = milli * 1000000L;
    if (nanosleep(&ts, &ts))
        ERR("nanosleep");
}

typedef struct {
    pthread_barrier_t barrier;
} DogTools_t;

void dog_work(DogTools_t* tool){
    pthread_barrier_wait(&tool->barrier);
    printf("[%d] waf waf waf (started race)\n", getpid());
    sleep(1);
}

void dog_breed(int N, DogTools_t* tool){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<N;i++){
        pid = fork();
        if(pid == 0){
            dog_work(tool);
            munmap(tool, sizeof(DogTools_t));
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }
}


int main(int argc, char** argv)
{
    if(argc != 3)
        usage(argv[0]);

    int L, N;
    L = atoi(argv[1]);
    N = atoi(argv[2]);

    if(L < MIN_TRACK_LEN || L > MAX_TRACK_LEN || N < MIN_DOG_COUNT || N > MAX_DOG_COUNT)
        usage(argv[0]);

    DogTools_t* tool;
    if(MAP_FAILED == (tool = (DogTools_t*)mmap(NULL, sizeof(DogTools_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)))
        ERR("mmap");

    pthread_barrierattr_t attr;
    pthread_barrierattr_setpshared(&attr,PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&tool->barrier, &attr, 1);
    dog_breed(N, tool);
    while(wait(NULL) > 0);
    pthread_barrier_destroy(&tool->barrier);
    pthread_barrierattr_destroy(&attr);
    munmap(tool, sizeof(DogTools_t));
    return 0;
}

// https://github.com/tpruvot/pthreads/blob/master/pthread_barrierattr_getpshared.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L3/PolishLab/sop-shop.c