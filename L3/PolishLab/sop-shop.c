#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHOP_FILENAME "./shop"
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
    pthread_mutex_t mutexes[MAX_SHELVES];
    int isSorted;
    pthread_mutex_t sortChecker;
    int aliveWorkers;
    pthread_mutex_t mxDeadWorkersCount;
}SharedData_t;

void shuffle(int* array, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        SWAP(array[i], array[j]);
    }
}

void fill_array(int* arr, int n){
    for (int i = 0; i < n; ++i)
    {
        arr[i] = i + 1;
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

void mutex_lock_handler(pthread_mutex_t* mutex, SharedData_t* sharedDt){
    int error;
    if ((error = pthread_mutex_lock(mutex)) != 0)
    {
        if (error == EOWNERDEAD)
        {
            printf("[%d] Found a dead body in aisle", getpid());
            pthread_mutex_lock(&sharedDt->mxDeadWorkersCount);
            sharedDt->aliveWorkers--;
            pthread_mutex_unlock(&sharedDt->mxDeadWorkersCount);
            pthread_mutex_consistent(mutex);
        }
        else
        {
            ERR("pthread_mutex_lock");
        }
    }
}

pthread_mutexattr_t init_mtx(){
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&mutexattr, PTHREAD_MUTEX_ROBUST);
    return mutexattr;
}

int isSorted(int* arr, int n){
    for (int i = 0; i < n; ++i)
    {
        if(arr[i] != i + 1) return 0;
    }
    return 1;
}

void manager_work(int productsNo, int* shop, SharedData_t* sharedDt){
    sharedDt->isSorted = 0;
    while(sharedDt->isSorted == 0){
        print_array(shop,productsNo);
        printf("[%d] Alive workers: %d\n", getpid(), sharedDt->aliveWorkers);
        if(sharedDt->aliveWorkers == 0){
            printf("[%d] All workers died, I hate my job\n", getpid());
            exit(0);
        }

        if(msync(shop, sizeof(int)*productsNo, MS_SYNC)<0){
            ERR("msync");
        }
        if(isSorted(shop, productsNo)){
            mutex_lock_handler(&sharedDt->sortChecker, sharedDt);
            sharedDt->isSorted = 1;
            pthread_mutex_unlock(&sharedDt->sortChecker);
            printf("[%d] The shop shelves are sorted\n", getpid());
        }
        msleep(500);
    }
}

void child_work(int productsNo, int* shop, SharedData_t* sharedDt){
    srand(getpid());
    int id1, id2;
    while(!sharedDt->isSorted){

        id1 = rand()%productsNo;
        while((id2 = rand()%productsNo) == id1);
        //printf("id1 = %d; id2 = %d\n", id1, id2); // just to check

        // make sure id1 is the smaller one
        if(id1 > id2) SWAP(id1,id2);

        if(rand() % 100 == 0){
            printf("[%d] Trips over pallet and dies\n", getpid());
            abort();
        }

        mutex_lock_handler(&sharedDt->mutexes[id1], sharedDt);
        mutex_lock_handler(&sharedDt->mutexes[id2], sharedDt);

        if(shop[id1] > shop[id2]) SWAP(shop[id1], shop[id2]);

        pthread_mutex_unlock(&sharedDt->mutexes[id1]);
        pthread_mutex_unlock(&sharedDt->mutexes[id2]);

        msleep(100);
    }
}

void create_workers(int productsNo, int workersNo, int* shop, SharedData_t* sharedDt){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<workersNo;i++){
        pid = fork();
        if(pid == 0){
            printf("[%d] Worker reports for a night shift\n", getpid());
            child_work(productsNo, shop, sharedDt);
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }

    // manager process
    pid = fork();
    if(pid == 0){
        printf("[%d] Manager reports for a night shift\n", getpid());
        manager_work(productsNo, shop, sharedDt);
        exit(0);
    }
    if(pid < 0){
        ERR("fork");
    }
}


int main(int argc, char** argv){
    if(argc !=3) usage(argv[0]);
    int productsNo = atoi(argv[1]);
    int workersNo = atoi(argv[2]);

    if(productsNo < 8 ||
        productsNo > 256 ||
        workersNo < 1 ||
        workersNo > 64)
    {
        usage(argv[0]);
    } 


    int fd;
    if((fd = open(SHOP_FILENAME, O_CREAT | O_RDWR | O_TRUNC, 0600)) < 0){
        ERR("open");
    }
    if(ftruncate(fd, sizeof(int)*productsNo) < 0){
        ERR("ftruncate");
    }

    int* ShopArray = (int*)mmap(NULL, sizeof(int)*productsNo, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(ShopArray == MAP_FAILED){
        ERR("mmap");
    }

    close(fd);

    SharedData_t* sharedDt;
    if((sharedDt = (SharedData_t*)mmap(NULL, sizeof(SharedData_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        ERR("mmap");
    }

    
    fill_array(ShopArray, productsNo);
    shuffle(ShopArray, productsNo);
    print_array(ShopArray,productsNo);

    pthread_mutexattr_t attr = init_mtx();
    for(int i=0;i<MAX_SHELVES;i++){
        pthread_mutex_init(&sharedDt->mutexes[i], &attr);
    }
    pthread_mutex_init(&sharedDt->sortChecker, &attr);
    pthread_mutex_init(&sharedDt->mxDeadWorkersCount, &attr);

    sharedDt->aliveWorkers = workersNo;

    create_workers(productsNo, workersNo, ShopArray, sharedDt);
    while(wait(NULL) > 0);
    print_array(ShopArray,productsNo);
    printf("Night shift in Bitronka is over\n");
    if(msync(ShopArray, sizeof(int)*productsNo, MS_SYNC)<0){
        ERR("msync");
    }
    for(int i=0;i<MAX_SHELVES;i++){
        pthread_mutex_destroy(&sharedDt->mutexes[i]);
    }
    pthread_mutex_destroy(&sharedDt->sortChecker);
    pthread_mutex_destroy(&sharedDt->mxDeadWorkersCount);
    pthread_mutexattr_destroy(&attr);
    munmap(ShopArray, sizeof(int)*productsNo);
    munmap(sharedDt, sizeof(SharedData_t));

    return 0;
}
