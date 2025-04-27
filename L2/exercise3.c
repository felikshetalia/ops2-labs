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

void child_work(){
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;
    attr.mq_curmsgs = 0;

    mqd_t queue;
    char qname[30];
    snprintf(qname, 30, "/sop_cwg_%d", getpid());
    if((queue = mq_open(qname, O_CREAT | O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
}

void create_children(int player_no, int t1, int t2, char** names){
    srand(getpid());
    for(int i = 0; i < player_no; i++){
        pid_t pid = fork();
        if(pid == 0){
            //child
            printf("[%d] {%s} has joined the game!\n",getpid(), names[i]);
            msleep(rand()% t2 + t1);
            printf("[%d] {%s} has left the game!\n",getpid(), names[i]);
            exit(0);
        }
        if(pid < 0) ERR("fork");

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
    //char **names = malloc(P*sizeof(char*));
    printf("Chinese Whispers Game\n");
    printf("Provide child name or type start to begin the game!\n");
    int player_no = 0;
    char *names[30];
    char line[256];
    char msg[256];

    while(fscanf(stdin, "%s", line) == 1){
        if(strcmp(line, "start") == 0){
            char tmp;
            while((tmp = fgetc(stdin)) != EOF && tmp != '"');
            // skip the start message and get to the message
            if(tmp == EOF)
            {
                fprintf(stderr, "Invalid input: expected opening quote\n");
                exit(EXIT_FAILURE);
            }
            char ch;
            int i = 0;
            while((ch = fgetc(stdin)) != EOF && ch != '"' && i < 255){
                msg[i++] = ch;
            }
            msg[i] = '\0'; // null terminate the msg
            if (ch != '"') {
                fprintf(stderr, "Invalid input: expected closing quote\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        else{
            // read names
            names[player_no] = strdup(line);
            if(!names[player_no]){
                ERR("strdup");
            }
            player_no++;
        }
    }
    create_children(player_no,t1,t2, names);
    while(wait(NULL) > 0);
    return EXIT_SUCCESS;
}