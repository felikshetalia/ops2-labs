#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t keepRunning = 1;

void handle_sigint(int sig) {
    keepRunning = 0;
}
void usage(char *name){
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "2<=n<=5 - 5<=m<=10\n");
    exit(EXIT_FAILURE);
}

void initialize_cards(int m, int**cards){
    *cards = malloc(m * sizeof(int));
    if (*cards == NULL) {
        ERR("malloc failed");
    }
    for(int i = 0; i < m; i++){
        (*cards)[i] = i+1;
    }
}

int find_max(int *scores, int n){
    int max = scores[0];
    for(int i = 0; i < n-1; i++){
        if(max <= scores[i+1]){
            max = scores[i+1];
        }
        else{
            continue;
        }
    }
    return max;
}

int main(int argc, char **argv){
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    if (argc != 3) {
        usage(argv[0]);
    }
    if(n < 2 || n > 5 || m < 5 || m > 10){
        usage(argv[0]);
    }
    int fd_send[n][2], fd_recv[n][2];
    // via fd_send child sends the card
    for(int i = 0; i < n; i++){
        if(pipe(fd_send[i]) < 0 || pipe(fd_recv[i]) < 0){
            ERR("pipe");
        }
    }
    int turn;
    int scores[n];
    memset(scores, 0, sizeof(scores));
    const char* new_round = "NEW ROUND";
    char msg_buffer[50];
    for(int i = 0; i < n; i++){
        switch (fork())
        {
        case -1:
            ERR("fork");
        case 0:
            //printf("Child %d entered\n", i+1);
            // if(close(fd_send[i][0]) < 0 || close(fd_recv[i][1]) < 0){
            //     ERR("close");
            // }
            int *cards;
            initialize_cards(m,&cards);
            // for(int i = 0; i < m; i++)
            //     printf("%d ", cards[i]);
            // printf("\n");
            srand(getpid());
            turn = rand()%(m-1);
            while (cards[turn] == 0) {  // Find an unused card
                turn = (turn + 1) % m;
            }
            int play = cards[turn];
            cards[turn] = 0; //set the card as used
            memset(msg_buffer, 0, 16);
            snprintf(msg_buffer, 16, "%d", play);
            if(write(fd_send[i][1], msg_buffer, 16) < 0){
                ERR("write");
            }
            while(read(fd_recv[i][0], msg_buffer, 16) > 0){
                if(strcmp(msg_buffer, new_round) == 0){
                    while (cards[turn] == 0) {  // Find an unused card
                        turn = (turn + 1) % m;
                    }
                    int play = cards[turn];
                    cards[turn] = 0; //set the card as used
                    memset(msg_buffer, 0, 16);
                    snprintf(msg_buffer, 16, "%d", play);
                    if(write(fd_send[i][1], msg_buffer, 16) < 0){
                        ERR("write");
                    }
                }
            }
            free(cards);
            if(close(fd_send[i][1]) < 0 || close(fd_recv[i][0]) < 0){
                ERR("close");
            }
            exit(0);
        }
    }
    // parent process
    for(int i = 0; i < m; i++){
        printf("ROUND [%d]\n", i+1);
        for (int i = 0; i < n; i++) {
            if(write(fd_recv[i][1], new_round, strlen(new_round) + 1) < 0){
                ERR("write");
            }
        }
        int max = 0;
        for (int i = 0; i < n; i++){
           char res[16];
           if(read(fd_send[i][0], res, 16) < 0){
            ERR("read");
           }
           else{
            int play = atoi(res);
            printf("Child %d sent %d\n", i+1, play);
            scores[i] = play;
           }
        }
        int winner_idx = 0;
        for (int i = 0; i < n; i++){
            if(scores[i] == find_max(scores,n)){
                winner_idx = i;
                printf("Congratulations child %d!\n", i+1);
            }
        }
    }
    while(wait(NULL) > 0);
    return 0;
}