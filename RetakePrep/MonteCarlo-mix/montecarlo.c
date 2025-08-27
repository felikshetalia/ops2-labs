#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#define MAX_ITER 100000

/**
 * It counts hit points by Monte Carlo method.
 * Use it to process one batch of computation.
 * @param N Number of points to randomize
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return Number of points which was hit.
 */
int getCircleHits()
{
    srand(getpid());
    int hits = 0;
    for (int i = 0; i < MAX_ITER; ++i)
    {
        float rand_x = (float)rand()/RAND_MAX;
        float rand_y = (float)rand()/RAND_MAX;
        // printf("x = %.6f; y = %.6f\n", rand_x, rand_y);

        if (rand_x*rand_x + rand_y*rand_y <= 1.0f)
            hits++;
    }
    return hits;
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

void child_work(int writeEnd){
    float pi = 4.0f * ((float)getCircleHits() / (float)MAX_ITER);
    if(write(writeEnd, &pi, sizeof(float)) < 0){
        if(errno != EPIPE)
            ERR("write");
    }                    
    printf("[%d] pi ≈ %.6f\n", getpid(), pi);
}

void create_workers(int N){
    srand(getpid());

    int* workerPipes;
    if((workerPipes = calloc(2*N, sizeof(int))) == NULL){
        ERR("malloc");
    }
    // rememebr to free
    for(int i = 0; i < N; i++){
        if(pipe(&workerPipes[2*i]) < 0){
            ERR("pipe");
        }
    }
    float sum = 0.0f;

    for(int i = 0; i < N; i++){
        pid_t pid = fork();
        if(pid < 0)
            ERR("fork");
        if(pid == 0){
            // child ...
            for(int j = 0; j < N; j++){
                if(i != j){
                    if(close(workerPipes[2*j+1]) < 0){
                        ERR("close");
                    }
                }
                if(close(workerPipes[2*j]) < 0){
                    ERR("close");
                }
            }
            child_work(workerPipes[2*i+1]);
            // printf("[%d] I'm ready to work\n", getpid());
            if(close(workerPipes[2*i+1]) < 0){
                ERR("close");
            }
            free(workerPipes);
            exit(0);
        }
        if(pid > 0)
        {
            // parent
            int ret;
            float pi_res;
            if((ret = read(workerPipes[2*i], &pi_res, sizeof(float))) < 0){
                ERR("read");
            }

            sum += pi_res;
            
        }
    }

    printf("[parent] mean ≈ %.6f from %d workers\n", (float)(sum/N), N);
    for(int i = 0; i < 2*N; i++){

        if(close(workerPipes[i]) < 0){
            ERR("close");
        }
    }
    free(workerPipes);
}

int main(int argc, char* argv[])
{
    if(argc != 2){
        usage(argv);
    }
    int N = atoi(argv[1]);
    if(N < 0 || N > 30){
        usage(argv);
    }
    create_workers(N);
    while(wait(NULL) >0);
    printf("Open descriptors: %d\n", count_descriptors());
    return EXIT_SUCCESS;
}