#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CHILDREN 5
#define MSG_SIZE 256

typedef struct {
    char info[MSG_SIZE];
} Message;

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Utility to generate queue names
void generate_queue_name(char *name, int id) {
    sprintf(name, "/child_queue_%d", id);
}

// Child process function
void child_process(int id, int num_children) {
    char my_queue_name[64];
    char next_queue_name[64];
    mqd_t my_queue, next_queue;

    generate_queue_name(my_queue_name, id);
    my_queue = mq_open(my_queue_name, O_RDONLY);
    if (my_queue == (mqd_t) -1) error_exit("Opening my queue");

    if (id < num_children - 1) {
        generate_queue_name(next_queue_name, id + 1);
        next_queue = mq_open(next_queue_name, O_WRONLY);
        if (next_queue == (mqd_t) -1) error_exit("Opening next queue");
    }

    Message msg;
    if (mq_receive(my_queue, (char*)&msg, sizeof(msg), NULL) == -1) {
        error_exit("Receiving message");
    }

    printf("Child %d received: %s\n", id, msg.info);

    // Modify the message or create a new one for the next child
    if (id < num_children - 1) {
        sprintf(msg.info, "Processed by %d", id);
        if (mq_send(next_queue, (char*)&msg, sizeof(msg), 0) == -1) {
            error_exit("Sending message to next child");
        }
        mq_close(next_queue);
    }

    mq_close(my_queue);
}

int main() {
    mqd_t mq[MAX_CHILDREN];
    char queue_name[64];
    struct mq_attr attr = {0, 10, sizeof(Message), 0};

    // Create queues for each child
    for (int i = 0; i < MAX_CHILDREN; i++) {
        generate_queue_name(queue_name, i);
        mq[i] = mq_open(queue_name, O_CREAT | O_RDWR, 0666, &attr);
        if (mq[i] == (mqd_t) -1) error_exit("Creating queue");
    }

    // Fork children
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (fork() == 0) {
            child_process(i, MAX_CHILDREN);
            exit(0);
        }
    }

    // Send initial message to the first child
    Message msg = {"Start processing"};
    if (mq_send(mq[0], (char*)&msg, sizeof(msg), 0) == -1) {
        error_exit("Sending initial message");
    }

    // Wait for all children to finish
    for (int i = 0; i < MAX_CHILDREN; i++) {
        wait(NULL);
    }

    // Cleanup queues
    for (int i = 0; i < MAX_CHILDREN; i++) {
        generate_queue_name(queue_name, i);
        mq_close(mq[i]);
        mq_unlink(queue_name);
    }

    return 0;
}
