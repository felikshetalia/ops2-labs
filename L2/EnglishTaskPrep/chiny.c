#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_PLAYERS 100
#define QUEUE_NAME "/eueu"
#define MSG_SIZE 256
#define ERR(source) \

    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void msleep(int millisec)
{
    struct timespec tt;
    tt.tv_sec = millisec / 1000;
    tt.tv_nsec = (millisec % 1000) * 1000000;
    while (nanosleep(&tt, &tt) == -1)
    {
    }
}

void usage(char *prog) {
    fprintf(stderr, "Usage: %s P T1 T2\n", prog);
    fprintf(stderr, "0 <= P <= 100, 100 <= T1 < T2 <= 6000\n");
    exit(EXIT_FAILURE);
}

void child_work(int child_no, int player_no, char *msg, mqd_t childq){
    // mqd_t child_now
    // mqd_t child_next;
    mqd_t coordinator_queue = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY);
    if(coordinator_queue < 0){
        ERR("mq_open");
    }
    struct mq_attr attr = {0, 10, MSG_SIZE, 0};  // Set message queue attributes
    char qname[MSG_SIZE];
    char next_qname[MSG_SIZE];

    snprintf(qname, MSG_SIZE, "/sop_cwg_%d", getpid());
    //pointer = &children_queues[0];
    // if((pointer = mq_open(childq, O_CREAT | O_RDONLY, 0600, &attr)) < 0){
    //     ERR("mq_open");
    // }
    // if (child_no < player_no - 1) {
    //     snprintf(next_qname, MSG_SIZE, "/sop_cwg_%d", getpid() + 1);  // Assuming sequential PIDs
    // }
    char recv[MSG_SIZE];
    unsigned int pr;
    if (mq_receive(coordinator_queue, recv, MSG_SIZE, &pr) == -1) {
        ERR("mq_receive");
    }
    printf("[%d] got the message: '%s'\n", getpid(), msg);
    // if(child_no < player_no - 1){
    //     if ((child_next = mq_open(next_qname, O_WRONLY)) == -1) {
    //         ERR("mq_open (next child)");
    //     }

    //     if (mq_send(child_next, msg, MSG_SIZE, 0) == -1) {
    //         ERR("mq_send");
    //     }
    //     mq_close(child_next);
    // }
    mq_close(coordinator_queue);
    mq_unlink(QUEUE_NAME);
}

void coordinator_work(mqd_t childq, char* msg){
    // struct mq_attr attr = {0, 10, MSG_SIZE, 0};
    // mqd_t coordinator_queue = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0600, &attr);
    
    // if(coordinator_queue < 0){
    //     ERR("mq_open");
    // }
    if(mq_send(childq, msg, MSG_SIZE, 0) < 0){
        ERR("mq_send");
    }
    //free(childq);
    mq_close(coordinator_queue);
}

void create_children(int player_no, int t1, int t2, char** names, char* msg, mqd_t* children_queues){
    srand(getpid());
    for(int i = 0; i < player_no; i++){
        pid_t pid = fork();
        if(pid == 0){
            //child
            printf("[%d] {%s} has joined the game!\n",getpid(), names[i]);
            msleep(rand()% t2 + t1);
            child_work(i,player_no, msg, children_queues[i]);
            printf("[%d] {%s} has left the game!\n",getpid(), names[i]);
            exit(0);
        }
        if(pid < 0) ERR("fork");
        else{
            coordinator_work(children_queues[i], msg);
        }

        free(names[i]);
    }
}

int main(int argc, char **argv)
{
    if(argc != 4) usage(argv[0]);
    int P = atoi(argv[1]);
    int t1 = atoi(argv[2]);
    int t2 = atoi(argv[3]);
    if(P < 0 || P > 100) usage(argv[0]);
    if(t1 < 100 || t1 > t2) usage(argv[0]);
    if(t2 < t1 || t2 > 6000) usage(argv[0]);
    mq_unlink(QUEUE_NAME);
    struct mq_attr attr = {0, 10, MSG_SIZE, 0};
    //char **names = malloc(P*sizeof(char*));
    printf("Chinese Whispers Game\n");
    printf("Provide child name or type start to begin the game!\n");

    int player_no = 0;
    char *names[30];
    char line[256];
    char msg[MSG_SIZE];

    while(fscanf(stdin, "%s", line) == 1){
        // read names
        names[player_no] = strdup(line);
        if(!names[player_no]){
            ERR("strdup");
        }
        player_no++;
        if(strcmp(line, "start") == 0){
            // retrieve the message
            char tmp;
            while((tmp = fgetc(stdin)) != EOF && tmp != '"');
            char ch;
            int i = 0;
            while((ch = fgetc(stdin)) != EOF && ch != '"' && i < 255){
                msg[i++] = ch;
            }
            msg[i] = '\0'; // null terminate the msg
            break;
        }
    }
    //printf("%s\n", msg);
    mqd_t *children_queues = calloc(player_no, sizeof(mqd_t));
    create_children(player_no,t1,t2, names, msg, children_queues);
    while(wait(NULL) > 0);
    // for(int i = 0; i < player_no; i++)
    //     free(names[i]);
    return EXIT_SUCCESS;
}