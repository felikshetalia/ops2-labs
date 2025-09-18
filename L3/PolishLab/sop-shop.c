#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHOP_FILENAME "/shop"
#define MIN_SHELVES 8
#define MAX_SHELVES 256
#define MIN_WORKERS 1
#define MAX_WORKERS 64

#define ERR(source)                                     \
    do                                                  \
    {                                                   \
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); \
        perror(source);                                 \
        kill(0, SIGKILL);                               \
        exit(EXIT_FAILURE);                             \
    } while (0)

#define SWAP(x, y)         \
    do                     \
    {                      \
        typeof(x) __x = x; \
        typeof(y) __y = y; \
        x = __y;           \
        y = __x;           \
    } while (0)

void usage(char* program_name)
{
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "\t%s n m\n", program_name);
    fprintf(stderr, "\t  n - number of items (shelves), %d <= n <= %d\n", MIN_SHELVES, MAX_SHELVES);
    fprintf(stderr, "\t  m - number of workers, %d <= m <= %d\n", MIN_WORKERS, MAX_WORKERS);
    exit(EXIT_FAILURE);
}

typedef struct {
    pthread_mutex_t shelfMutexes[MAX_SHELVES];
}shared_t;

void ms_sleep(unsigned int milli)
{
    time_t sec = (int)(milli / 1000);
    milli = milli - (sec * 1000);
    struct timespec ts = {0};
    ts.tv_sec = sec;
    ts.tv_nsec = milli * 1000000L;
    if (nanosleep(&ts, &ts))
        ERR("nanosleep");
}

void shuffle(int* array, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        SWAP(array[i], array[j]);
    }
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
        array[i] = i+1;
    }
}

pthread_mutexattr_t init_mutex(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

    return attr;
}

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

void worker_thread(int N, int* shopArray, shared_t* sharedStock){
    srand(getpid());
    int idx1, idx2 = -1;
    for(int i=0;i<10;i++){
        idx1 = rand() % N;
        while((idx2 = rand() % N) == idx1);

        if(idx2 < idx1)
            SWAP(idx2, idx1);

        pthread_mutex_lock(&sharedStock->shelfMutexes[idx1]);
        pthread_mutex_lock(&sharedStock->shelfMutexes[idx2]);

        if(shopArray[idx1] > shopArray[idx2]){
            SWAP(shopArray[idx1], shopArray[idx2]);
            ms_sleep(100);
        }
        
        pthread_mutex_unlock(&sharedStock->shelfMutexes[idx1]);
        pthread_mutex_unlock(&sharedStock->shelfMutexes[idx2]);
        print_array(shopArray, N);
        ms_sleep(1000);
    }
}

void create_workers(int M, int N, int* shopArray, shared_t* sharedStock){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<M;i++){
        pid = fork();
        if(pid == 0){
            printf("[%d] Worker reports for a night shift\n", getpid());
            worker_thread(N, shopArray, sharedStock);
            munmap(shopArray, N*sizeof(int));
            munmap(sharedStock, sizeof(shared_t));
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }
}

int main(int argc, char** argv) { 
    shm_unlink(SHOP_FILENAME);
    if(argc !=3)
        usage(argv[0]);

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    if(N < MIN_SHELVES || N > MAX_SHELVES)
        usage(argv[0]);

    if(M < MIN_WORKERS || M > MAX_WORKERS)
        usage(argv[0]);

    int shfd;
    if(0 > (shfd = shm_open(SHOP_FILENAME, O_CREAT | O_EXCL | O_RDWR, 0600))){
        if(errno == EEXIST){
            if(0 > (shfd = shm_open(SHOP_FILENAME, O_RDWR, 0600))){
                shm_unlink(SHOP_FILENAME);
                ERR("shm_open");
            }
        }
        else{
            shm_unlink(SHOP_FILENAME);
            ERR("shm_open");
        }
    }
    else{
        if(0 > ftruncate(shfd, sizeof(int))){
            ERR("ftruncate");
        }
    }

    int* shopArray;
    if(MAP_FAILED == (shopArray = (int*)mmap(NULL, sizeof(int)*N, PROT_READ | PROT_WRITE, MAP_SHARED, shfd, 0))){
        ERR("mmap");
    }

    if(0 > close(shfd)) ERR("close");

    shared_t* sharedStock;
    if(MAP_FAILED == (sharedStock = (shared_t*)mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))){
        ERR("mmap");
    }

    fill_array(shopArray, N);
    shuffle(shopArray, N);
    print_array(shopArray,N);

    pthread_mutexattr_t attr = init_mutex();
    for(int i=0;i<MAX_SHELVES;i++){
        pthread_mutex_init(&sharedStock->shelfMutexes[i], &attr);
    }

    create_workers(M,N, shopArray,sharedStock);

    while(wait(NULL) > 0);
    printf("Night shift in Bitronka is over\n");
    print_array(shopArray,N);
    munmap(shopArray, N*sizeof(int));
    for(int i=0;i<MAX_SHELVES;i++){
        pthread_mutex_destroy(&sharedStock->shelfMutexes[i]);
    }
    munmap(sharedStock, sizeof(shared_t));
    pthread_mutexattr_destroy(&attr);
    shm_unlink(SHOP_FILENAME);
    return EXIT_SUCCESS;
}
