#include <pthread.h>
#include <stdio.h>

#define N 3
pthread_barrier_t barrier;

void* task(void* arg) {
    printf("Thread %ld is ready\n", (long)arg);
    pthread_barrier_wait(&barrier);
    printf("Thread %ld passed the barrier\n", (long)arg);
    return NULL;
}

int main() {
    pthread_t threads[N];
    pthread_barrier_init(&barrier, NULL, N);
    for (long i = 0; i < N; i++)
        pthread_create(&threads[i], NULL, task, (void*)i);

    for (int i = 0; i < N; i++)
        pthread_join(threads[i], NULL);

    pthread_barrier_destroy(&barrier);
    return 0;
}
