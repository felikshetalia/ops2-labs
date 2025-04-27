#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THREAD_COUNT 3

pthread_barrier_t barrier;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

void* worker(void* arg) {
    int id = *(int*)arg;
    printf("Thread %d: doing some initial work...\n", id);
    sleep(2);  // Simulate work

    printf("Thread %d: waiting at the barrier\n", id);
    pthread_barrier_wait(&barrier);

    pthread_mutex_lock(&cond_mutex);
    while (!ready) {
        printf("Thread %d: waiting on condition variable...\n", id);
        pthread_cond_wait(&cond, &cond_mutex);
    }
    pthread_mutex_unlock(&cond_mutex);

    printf("Thread %d: resuming after condition broadcast!\n", id);
    return NULL;
}

int main() {
    srand(getpid());
    pthread_t threads[THREAD_COUNT];
    int ids[THREAD_COUNT];

    pthread_barrier_init(&barrier, NULL, THREAD_COUNT);

    // Create threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Wait a bit and then signal the condition variable
    sleep(3);
    printf("Main thread: all threads reached the barrier, signaling condition...\n");

    pthread_mutex_lock(&cond_mutex);
    ready = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&cond_mutex);

    // Join all threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&cond_mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

// 💡 Summary:
// pthread_create() = "Start this thread now."

// pthread_join() = "Wait for it to finish later."

// So your worker() function starts running as soon as the thread is created.

// Let me know if you'd like a visual timeline of how the barrier and condition flow together!