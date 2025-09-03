#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SWAP(x, y)         \
do                     \
{                      \
    typeof(x) __x = x; \
    typeof(y) __y = y; \
    x = __y;           \
    y = __x;           \
} while (0)


void usage(char *name){
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "4<=n<=7 \n m>=4\n n*m <= 52\n");
    exit(EXIT_FAILURE);
}

typedef struct {
    int* cards;
    pid_t pid;
} player_t;

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

void deal_random_cards(int* wholeDeck, int* playerCards, int M){
    srand(getpid());
    for(int i = 0; i < M; i++){
        int randIdx = rand() % 52;
        int prevIdx;
        if(wholeDeck[randIdx]){
            playerCards[i] = wholeDeck[randIdx];
            wholeDeck[randIdx] = 0;
            prevIdx = randIdx;
        }
        else{
            prevIdx = randIdx;
            srand(getpid());
            while((randIdx = rand() % 52) == prevIdx);
        }
    }

}

void create_players(int N, int M, player_t* playersList, int* deck){
    srand(getpid());

    int *playerPipes;
    if((playerPipes = calloc(2*N, sizeof(int))) == NULL){
        ERR("calloc");
    }

    pid_t pid;

    for(int i = 0; i < N; i++){
        //create children
        pid = fork();
        if(pid < 0) ERR("fork");
        if(pid == 0){
            //child runs
            free(playerPipes);
            exit(0);
        }
    }
    free(playerPipes);

}

int main(int argc, char **argv){
    if(argc!=3)
        usage(argv[0]);

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    if(N < 4 || N > 7 || M < 4)
        usage(argv[0]);

    if(M*N > 52)
        usage(argv[0]);

    // data structure: int array of size M
    player_t* playersList;
    if((playersList = calloc(N, sizeof(player_t))) == NULL){
        ERR("calloc");
    }

    int deck[52] = {0};
    fill_array(deck, 52);
    shuffle(deck, 52);
    // print_array(deck, 52);
    for(int i=0;i<N;i++){
        if((playersList[i].cards = calloc(M, sizeof(int))) == NULL){
            ERR("calloc");
        }
    }
    for(int i=0;i<N;i++){
        srand(getpid());
        deal_random_cards(deck, playersList[i].cards, M);
        print_array(playersList[i].cards, M);
    
        printf("\n\n");
    }

    create_players(N,M,playersList, deck);
    while(wait(NULL) > 0);
    for(int i=0;i<N;i++){
        free(playersList[i].cards);
    }
    free(playersList);
    return 0;
}