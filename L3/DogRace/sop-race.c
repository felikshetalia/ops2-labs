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
void print_array(int* array, int n)
{
    for (int i = 0; i < n; ++i)
    {
        printf("%3d ", array[i]);
    }
    printf("\n");
}
void fill_array(int* array, int n)
{
    for (int i = 0; i < n; ++i)
    {
        array[i] = 0;
    }
}

pthread_mutexattr_t init_mutex(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

    return attr;
}

typedef struct {
    pthread_barrier_t barrier;
    pthread_mutex_t dogMutexes[MAX_DOG_COUNT];
} DogTools_t;

void dog_work(DogTools_t* tool, int* raceTrack, int dogIndex, int L){
    srand(getpid());
    pthread_barrier_wait(&tool->barrier);
    printf("[%d] waf waf waf (started race)\n", getpid());
    int pos = 0;
    raceTrack[pos]=getpid();
    sleep(1);

    while(1)
    {
        int move = rand() % (MAX_MOVEMENT - MIN_MOVEMENT + 1) + MIN_MOVEMENT;
        int dogPID = getpid();
        pos += move;
        if(pos > L){
            pos = 2*L - pos - move;
            raceTrack[pos]
        }
        if(raceTrack[pos + move] == 0){
            raceTrack[pos] = 0;
            raceTrack[pos+move] = dogPID;
            msleep(rand()%(MAX_SLEEP - MIN_SLEEP + 1) + MIN_SLEEP);
        }
        else{
            printf("waf waf waf\n");
        }
    }
}

void dog_breed(int N, int L, DogTools_t* tool, int* raceTrack){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<N;i++){
        pid = fork();
        if(pid == 0){
            dog_work(tool, raceTrack, i);
            print_array(raceTrack, L);
            munmap(tool, sizeof(DogTools_t));
            munmap(raceTrack, sizeof(int)*L);
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

    int *raceTrack;
    if(MAP_FAILED == (raceTrack = (int*)mmap(NULL, L * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)))
        ERR("mmap");

    fill_array(raceTrack,L);
    pthread_barrierattr_t attr;
    pthread_barrierattr_setpshared(&attr,PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&tool->barrier, &attr, 1);

    pthread_mutexattr_t mutexAttr = init_mutex();
    for(int i=0;i<MAX_DOG_COUNT;i++){
        pthread_mutex_init(&tool->dogMutexes[i], &mutexAttr);
    }

    // main
    dog_breed(N, L, tool, raceTrack);
    while(wait(NULL) > 0);

    // cleanup
    pthread_barrier_destroy(&tool->barrier);
    pthread_barrierattr_destroy(&attr);
    for(int i=0;i<MAX_DOG_COUNT;i++){
        pthread_mutex_destroy(&tool->dogMutexes[i]);
    }
    pthread_mutexattr_destroy(&mutexAttr);
    munmap(tool, sizeof(DogTools_t));
    munmap(raceTrack, sizeof(int)*L);
    return 0;
}

// https://github.com/tpruvot/pthreads/blob/master/pthread_barrierattr_getpshared.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L3/PolishLab/sop-shop.c