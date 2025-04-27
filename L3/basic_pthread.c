#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define ITERATIONS 10000000

// atomic_int flag[2] = {0,0};
// atomic_int turn = 0;
 
// void lock(int i){
//     int j = 1 - i; // either 0 or 1
//     flag[i] = 1;
//     turn = j;
//     while(flag[j] && turn == j);
// }

// void unlock(int i)
// {
//     flag[i] = 0;
// }

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t semawhore;
void *producer(void *arg) {
    int *counter = (int *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        //lock(0);
        // pthread_mutex_lock(&mutex);
        sem_wait(&semawhore);
        (*counter)++;
        //unlock(0);
        // pthread_mutex_unlock(&mutex);
        sem_post(&semawhore);
    }

    return NULL;
}

void *consumer(void *arg) {
    int *counter = (int *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        //lock(1);
        //pthread_mutex_lock(&mutex);
        sem_wait(&semawhore);
        (*counter)--;
        //unlock(1);
        //pthread_mutex_unlock(&mutex);
        sem_post(&semawhore);

    }

    return NULL;
}

int main() {

    pthread_t pro, cons;
    int count = 0;
    if((pro = pthread_create(&pro, NULL, producer, &count)) < 0){
        ERR("pthread_create");
    }
    if((cons = pthread_create(&cons, NULL, consumer, &count)) < 0){

        ERR("pthread_create");

    }
    pthread_join(pro, NULL);
    pthread_join(cons, NULL);
    printf("Counter: %d\n", count);

    return 0;
}