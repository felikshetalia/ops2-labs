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

typedef struct
{
    // pthread_barrier_t barr[MAX_DOG_COUNT];
    pthread_barrier_t barr;

    pthread_mutex_t runMutex;
} SharedZone_t;

// typedef struct
// {
//     int dogPid;
// } TrackSegment_t;

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

pthread_mutexattr_t init_mtx()
{
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&mutexattr, PTHREAD_MUTEX_ROBUST);
    return mutexattr;
}

void mutex_lock_handler(pthread_mutex_t* mutex)
{
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

void dog_work(SharedZone_t* sharedDt, int* track, int L)
{
    pthread_barrier_wait(&sharedDt->barr);
    printf("Dog [%d] waiting...\n", getpid());
    sleep(1);

    for (int i = 0; i < L; i++)
    {
        track[i] = 0;
    }
    // each dog will move here
    while (1)
    {
        msleep(rand() % (MAX_SLEEP - MIN_SLEEP + 1) + MIN_SLEEP);
        int move = rand() % (MAX_MOVEMENT -  MIN_MOVEMENT + 1) + MIN_MOVEMENT;
        int dogPid = getpid();
        int curridx = 0;
        curridx += move;
        mutex_lock_handler(&sharedDt->runMutex);
        if (curridx <= MAX_TRACK_LEN)
        {
            if (track[curridx] > 0)
            {
                printf("[%d] waf waf (occupied)", getpid());
                track[curridx] = dogPid;
            }
        }

        pthread_mutex_unlock(&sharedDt->runMutex);

        printf("[%d] Dog is at pos [%d]", getpid(), curridx);
    }
}

void birth_dogs(int dogsNo, SharedZone_t* sharedDt, int* track, int L)
{
    srand(getpid());
    pid_t pid;
    for (int i = 0; i < dogsNo; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            dog_work(sharedDt, track, L);
            printf("[%d] waf waf (started race)\n", getpid());
            munmap(sharedDt, sizeof(SharedZone_t));
            // munmap(track, sizeof(TrackSegment_t) * racetrackLen);
            munmap(track, sizeof(int) * L);

            exit(0);
        }
        if (pid < 0)
        {
            ERR("fork");
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
        usage(argv[0]);
    int racetrackLen = atoi(argv[1]);
    int dogsNo = atoi(argv[2]);
    if (racetrackLen < 16 || racetrackLen > 256 || dogsNo < 2 || dogsNo > 6)
    {
        usage(argv[0]);
    }

    SharedZone_t* sharedDt;
    if ((sharedDt = (SharedZone_t*)mmap(NULL, sizeof(SharedZone_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,
                                        -1, 0)) == MAP_FAILED)
    {
        ERR("mmap");
    }
    int* track;
    if ((track = (int*)mmap(NULL, sizeof(int) * racetrackLen, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1,
                            0)) == MAP_FAILED)
    {
        ERR("mmap");
    }

    // pthread_mutexattr_t mutexattr = init_mtx();
    // pthread_mutex_init(&sharedDt->runMutex, &mutexattr);
    pthread_barrierattr_t attr;
    pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    // for (int i = 0; i < MAX_DOG_COUNT; i++)
    //     pthread_barrier_init(&sharedDt->barr[i], &attr, dogsNo);

    pthread_barrier_init(&sharedDt->barr, &attr, dogsNo);

    birth_dogs(dogsNo, sharedDt, track, racetrackLen);
    while (wait(NULL) > 0)
        ;

    // if(msync(sharedDt, sizeof(SharedZone_t), MS_SYNC)<0){
    //     ERR("msync");
    // }
    // for (int i = 0; i < MAX_DOG_COUNT; i++)
    //     pthread_barrier_destroy(&sharedDt->barr[i]);

    pthread_barrier_destroy(&sharedDt->barr);
    pthread_barrierattr_destroy(&attr);
    // pthread_mutex_destroy(&sharedDt->runMutex);
    // pthread_mutexattr_destroy(&mutexattr);
    munmap(sharedDt, sizeof(SharedZone_t));
    // munmap(sharedDt, sizeof(pid_t) - 1);
    munmap(track, sizeof(int) * racetrackLen);
    return 0;
}

// https://github.com/tpruvot/pthreads/blob/master/pthread_barrierattr_getpshared.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L3/PolishLab/sop-shop.c