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

typedef struct {
    pthread_barrier_t barr;
    pthread_mutex_t dogMutexes[MAX_DOG_COUNT];
    int returnFlags[MAX_DOG_COUNT];
} shared_t;

pthread_mutexattr_t init_mutex(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

    return attr;
}

pthread_barrierattr_t init_barrier(){
    pthread_barrierattr_t attr;
    pthread_barrierattr_init(&attr);
    pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    return attr;
}

void dog_work(shared_t* stock, int* track, int L, int dogIndex){
    srand(getpid());
    pthread_barrier_wait(&stock->barr);
    printf("[%d] waf waf waf (started race)\n", getpid());
    msleep(1000);
    int pos = 0;
    track[pos] = getpid();
    int prevPos = pos;
    while(1)
    {
        int move = rand() % (MAX_MOVEMENT - MIN_MOVEMENT + 1) + MIN_MOVEMENT;
        
        if((pos + move) < L){
            // move forward
            pthread_mutex_lock(&stock->dogMutexes[dogIndex]);
            prevPos = pos;
            stock->returnFlags[dogIndex] = 0;
            pos += move;
            pthread_mutex_unlock(&stock->dogMutexes[dogIndex]);
            if(track[pos] == 0){
                // empty
                pthread_mutex_lock(&stock->dogMutexes[dogIndex]);
                track[pos] = getpid();
                track[prevPos] = 0;
                pthread_mutex_unlock(&stock->dogMutexes[dogIndex]);
                printf("[%d] Position = %d\n", getpid(), pos);
                print_array(track, L);
            }
            else{
               printf("[%d] waf waf waf\n", getpid()); 
            }
        }
        else{
            // move back
            pthread_mutex_lock(&stock->dogMutexes[dogIndex]);
            prevPos = pos;
            stock->returnFlags[dogIndex] = 1;
            pthread_mutex_unlock(&stock->dogMutexes[dogIndex]);
            if(stock->returnFlags[dogIndex]) { 
                pthread_mutex_lock(&stock->dogMutexes[dogIndex]);
                pos = 2*(L-1) - pos - move;
                stock->returnFlags[dogIndex] = 0;
                if(track[pos] == 0){
                    // empty
                    track[pos] = getpid();
                    track[prevPos] = 0;
                    pthread_mutex_unlock(&stock->dogMutexes[dogIndex]);
                    printf("[%d] Position = %d + %d\n", getpid(), L, move);
                    print_array(track, L);
                }
                
            }
            else{
                pthread_mutex_lock(&stock->dogMutexes[dogIndex]);
                pos = pos - move;
                if(track[pos] == 0){
                    // empty
                    track[pos] = getpid();
                    track[prevPos] = 0;
                    pthread_mutex_unlock(&stock->dogMutexes[dogIndex]);
                    printf("[%d] Position = %d + %d\n", getpid(), L, pos);
                    print_array(track, L);
                }
                else{
                    printf("[%d] waf waf waf\n", getpid()); 
                }
            }

        }
        msleep(rand() % (MAX_SLEEP - MIN_SLEEP + 1) + MIN_SLEEP);
    }
}

void bring_dogs(int DOGS, shared_t* stock, int* track, int L){
    fill_array(track, L);
    srand(getpid());
    pid_t pid;
    for(int i=0;i<DOGS;i++){
        pid = fork();
        if(pid == 0){
            dog_work(stock, track, L, i);
            munmap(stock, sizeof(shared_t));
            munmap(track, L*sizeof(int));
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }
}

int main(int argc, char** argv)
{
    if(argc !=3)
        usage(argv[0]);

    int LEN = atoi(argv[1]);
    int DOGS = atoi(argv[2]);

    if(LEN < MIN_TRACK_LEN || LEN > MAX_TRACK_LEN)
        usage(argv[0]);

    if(DOGS < MIN_DOG_COUNT || DOGS > MAX_DOG_COUNT)
        usage(argv[0]);

    shared_t* sharedStock;
    if(MAP_FAILED == (sharedStock = (shared_t*)mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))){
        ERR("mmap");
    }

    int* track;
    if(MAP_FAILED == (track = (int*)mmap(NULL, sizeof(int)*LEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))){
        ERR("mmap");
    }

    pthread_barrierattr_t barrierAttr = init_barrier();
    pthread_mutexattr_t mutexAttr = init_mutex();
    pthread_barrier_init(&sharedStock->barr, &barrierAttr, DOGS);
    for(int i=0;i<MAX_DOG_COUNT;i++){
        pthread_mutex_init(&sharedStock->dogMutexes[i], &mutexAttr);
    }
    bring_dogs(DOGS, sharedStock, track, LEN);
    while(wait(NULL) > 0);

    // cleanup
    pthread_barrier_destroy(&sharedStock->barr);
    pthread_barrierattr_destroy(&barrierAttr);
    for(int i=0;i<MAX_DOG_COUNT;i++){
        pthread_mutex_destroy(&sharedStock->dogMutexes[i]);
    }
    pthread_mutexattr_destroy(&mutexAttr);
    munmap(sharedStock, sizeof(shared_t));
    munmap(track, LEN*sizeof(int));
    return 0;
}

// https://github.com/tpruvot/pthreads/blob/master/pthread_barrierattr_getpshared.c
// https://github.com/felikshetalia/ops2-labs/blob/main/L3/PolishLab/sop-shop.c