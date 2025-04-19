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
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <semaphore.h>

#define SHM_NAME "/salak"
#define SEM_NAME "/whore"

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

// Values of this function are in range (0,1]
double func(double x)
{
    usleep(2000);
    return exp(-x * x);
}

/**
 * It counts hit points by Monte Carlo method.
 * Use it to process one batch of computation.
 * @param N Number of points to randomize
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return Number of points which was hit.
 */
int randomize_points(int N, float a, float b)
{
    int result = 0;
    for (int i = 0; i < N; ++i)
    {
        double rand_x = ((double)rand() / RAND_MAX) * (b - a) + a;
        double rand_y = ((double)rand() / RAND_MAX);
        double real_y = func(rand_x);

        if (rand_y <= real_y)
            result++;
    }
    return result;
}

/**
 * This function calculates approximation of integral from counters of hit and total points.
 * @param total_randomized_points Number of total randomized points.
 * @param hit_points Number of hit points.
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return The approximation of integral
 */
double summarize_calculations(uint64_t total_randomized_points, uint64_t hit_points, float a, float b)
{
    return (b - a) * ((double)hit_points / (double)total_randomized_points);
}

/**
 * This function locks mutex and can sometime die (it has 2% chance to die).
 * It cannot die if lock would return an error.
 * It doesn't handle any errors. It's users responsibility.
 * Use it only in STAGE 4.
 *
 * @param mtx Mutex to lock
 * @return Value returned from pthread_mutex_lock.
 */
int random_death_lock(pthread_mutex_t* mtx)
{
    int ret = pthread_mutex_lock(mtx);
    if (ret)
        return ret;

    // 2% chance to die
    if (rand() % 50 == 0)
        abort();
    return ret;
}

void usage(char* argv[])
{
    printf("%s a b N - calculating integral with multiple processes\n", argv[0]);
    printf("a - Start of segment for integral (default: -1)\n");
    printf("b - End of segment for integral (default: 1)\n");
    printf("N - Size of batch to calculate before reporting to shared memory (default: 1000)\n");
}

typedef struct {
    int counter;
    pthread_mutex_t mutex;
}pack_t;

pthread_mutexattr_t init_mutex(pack_t* obj){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&obj->mutex, &attr);
    obj->counter = 0;
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

void create_processes(int n){
    srand(getpid());
    for (int i = 0; i < n; i++){
        pid_t pid = fork();
        if(pid == 0){
            // child
            printf("Process %d has joined!\n", i+1);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
}


int main(int argc, char** argv)
{
    srand(getpid());
    if(argc != 4) usage(argv);
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int N = atoi(argv[3]);

    //shm_unlink(SHM_NAME); // make sure
    int first_mem = 0;

    sem_t *semawhore;
    if(SEM_FAILED == (semawhore = sem_open(SEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666, 1))){
        if(errno == EEXIST){
            if(SEM_FAILED == (semawhore = sem_open(SEM_NAME, O_RDWR, 0666, 1))){
                ERR("sem_open");
            }
        }
        else
            ERR("sem_open");
    }
    sem_wait(semawhore);
    int shfd;
    if(0 > (shfd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600))){
        if(errno == EEXIST){
            if(0 > (shfd = shm_open(SHM_NAME, O_RDWR, 0600))){
                sem_post(semawhore);
                ERR("shm_open");
            }
        }
        else{
            sem_post(semawhore);
            ERR("shm_open");
        }
    }
    else{
        if(0 > ftruncate(shfd, sizeof(pack_t))){
            sem_post(semawhore);
            ERR("ftruncate");
        }
        first_mem = 1;
    }
    
    pack_t* obj;
    if(MAP_FAILED == (obj = (pack_t*)mmap(NULL, sizeof(pack_t), PROT_READ | PROT_WRITE, MAP_SHARED, shfd, 0))){
        sem_post(semawhore);
        ERR("mmap");
    }
    sem_post(semawhore);
    if(first_mem){
        memset(obj, 0, sizeof(pack_t));
        pthread_mutexattr_t attr = init_mutex(obj);
    }

    //printf("Program runs!\n");
    mutex_lock_handler(&obj->mutex);
    obj->counter++;
    msync(obj, sizeof(pack_t), MS_SYNC);
    usleep(5000);
    printf("Active workers: %d\n", obj->counter);
    pthread_mutex_unlock(&obj->mutex);


    // exiting processes

    usleep(200000);

    mutex_lock_handler(&obj->mutex);
    obj->counter--;
    int last = (obj->counter == 0);
    pthread_mutex_unlock(&obj->mutex);

    if (last) {
        printf("Last process exiting. Cleaning up shared resources...\n");
        if (munmap(obj, sizeof(pack_t)) == -1) ERR("munmap");
        if (shm_unlink(SHM_NAME) == -1) ERR("shm_unlink");
        if (sem_close(semawhore) == -1) ERR("sem_close");
        if (sem_unlink(SEM_NAME) == -1) ERR("sem_unlink");
    } else {
        if (munmap(obj, sizeof(pack_t)) == -1) ERR("munmap");
        if (sem_close(semawhore) == -1) ERR("sem_close");
    }

    // if(sem_close(semawhore) > 0){
    //     ERR("sem_close");
    // }
    // shm_unlink("/salak");
    // sem_unlink("/whore");
    return 0;
}

