#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHOP_FILENAME "/tmp/shop"
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

typedef struct DATA_COMMON{
    pthread_mutex_t MutexesArray[MAX_SHELVES];
}DATA_COMMON;

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

pthread_mutexattr_t init_mutex(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    //pthread_mutex_init(&obj->data_mtx, &attr);
    return attr;
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

void child_work(int n, int* ShopArray, DATA_COMMON* sharedData)
{
    srand(getpid());

    int a;
    for(int i=0;i<10;i++){
        int b = rand() % n + 1;
        while((a = rand() % n + 1) == b);
            
        if(a > b)
            SWAP(a,b);
        if(ShopArray[a-1] > ShopArray[b-1]){
            mutex_lock_handler(&sharedData->MutexesArray[a]);
            mutex_lock_handler(&sharedData->MutexesArray[b]);

            SWAP(ShopArray[a-1], ShopArray[b-1]);
            
            pthread_mutex_unlock(&sharedData->MutexesArray[a]);
            pthread_mutex_unlock(&sharedData->MutexesArray[b]);

        }
        ms_sleep(100);
    }

}

void create_children(int m, int n, int* ShopArray, DATA_COMMON* sharedData){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<m;i++){
        pid = fork();
        if(pid == 0){
            printf("[%d] Worker reports for a night shift\n", getpid());
            child_work(n, ShopArray, sharedData);
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        usage(argv[0]);
    }
    int productsNo = atoi(argv[1]);
    int workersNo = atoi(argv[2]);

    if(productsNo < 8 || productsNo > 256 || workersNo < 1 || workersNo > 64){
        usage(argv[0]);
    }

    int fd = open(SHOP_FILENAME, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if(fd < 0)
        ERR("open");

    if(ftruncate(fd, productsNo*sizeof(int)) < 0)
        ERR("ftruncate");
    int *ShopArray = (int*)mmap(NULL, productsNo*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(MAP_FAILED == ShopArray)
        ERR("mmap");

    close(fd);

    DATA_COMMON *sharedData = (DATA_COMMON*)mmap(NULL, sizeof(DATA_COMMON), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(MAP_FAILED == sharedData)
        ERR("mmap");

    pthread_mutexattr_t attr = init_mutex();
    for(int i = 0; i < MAX_SHELVES; i++)
    {
        pthread_mutex_init(&sharedData->MutexesArray[i], &attr);
    }

    for(int i = 0; i < productsNo; i++)
    {
        ShopArray[i] = i+1;
    }
    shuffle(ShopArray, productsNo);
    print_array(ShopArray, productsNo);

    create_children(workersNo, productsNo, ShopArray, sharedData);
    
    while(wait(NULL) > 0);
    print_array(ShopArray, productsNo);
    printf("Night shift at bitronka is over\n");
    for(int i = 0; i < MAX_SHELVES; i++)
    {
        pthread_mutex_destroy(&sharedData->MutexesArray[i]);
    }
    pthread_mutexattr_destroy(&attr);
    if(munmap(ShopArray, productsNo*sizeof(int)) < 0)
        ERR("munmap");
    if(msync(ShopArray, productsNo*sizeof(int), MS_SYNC))
    if(munmap(sharedData, sizeof(DATA_COMMON)) < 0)
        ERR("munmap");

    return 0;
}