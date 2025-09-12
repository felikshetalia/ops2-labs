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

int count_descriptors()
{
    int count = 0;
    DIR* dir;
    struct dirent* entry;
    struct stat stats;
    if ((dir = opendir("/proc/self/fd")) == NULL)
        ERR("opendir");
    char path[PATH_MAX];
    getcwd(path, PATH_MAX);
    chdir("/proc/self/fd");
    do
    {
        errno = 0;
        if ((entry = readdir(dir)) != NULL)
        {
            if (lstat(entry->d_name, &stats))
                ERR("lstat");
            if (!S_ISDIR(stats.st_mode))
                count++;
        }
    } while (entry != NULL);
    if (chdir(path))
        ERR("chdir");
    if (closedir(dir))
        ERR("closedir");
    return count - 1;  // one descriptor for open directory
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
        int randIdx;
        do {
            randIdx = rand() % 52;
        } while (wholeDeck[randIdx] == 0);

        playerCards[i] = wholeDeck[randIdx];
        wholeDeck[randIdx] = 0;   // mark taken
    }

}

int isWinning(int* cards, int M)
{
    for(int i = 0; i < M-1; i++){
        if((cards[i]%4) != (cards[i+1]%4))
            return 0;
    }
    return 1;
}

void child_work(player_t player, int writeEndNext, int readEndPrev, int M, int sharedWriteEnd){
    srand(getpid());
    while(1)
    {    
        // loop for sending
        
        

            int randIdx;
            do{
                randIdx = rand() % M;
            }while(player.cards[randIdx] == 0);
    
            int cardToDiscard = player.cards[randIdx];
            player.cards[randIdx] = 0;
    
            if(write(writeEndNext, &cardToDiscard, sizeof(int)) < 0){
                if(errno == EPIPE) break;
    
                ERR("write");
            }
            printf("[%d] Sent %d\n", player.pid, cardToDiscard);
            
            break;
            
        }
        // loop for receiving
        while(1){

            int cardReceived;
            ssize_t ret;
            if((ret = read(readEndPrev, &cardReceived, sizeof(int))) < 0){
                if(errno == EAGAIN) break;
                
                ERR("read");
            }
            if(ret){
                int i = -1;
                while(player.cards[++i] != 0);
                player.cards[i] = cardReceived;
                printf("[%d] Received %d\n", player.pid, cardReceived);
            }
            if(isWinning(player.cards, M)){
                printf("[%d]: My ship sails!\n", player.pid);
                if(write(sharedWriteEnd, &(player.pid), sizeof(pid_t)) < 0){
                    ERR("write");
                }
                break;
            }
            sleep(1);
            break;
        }

    }
}

void create_players(int N, int M, player_t* playersList, int* deck){
    srand(getpid());

    int *playerPipes;
    if((playerPipes = calloc(2*N, sizeof(int))) == NULL){
        ERR("calloc");
    }

    int SHARED[2];
    if(pipe(SHARED) < 0){
        ERR("pipe");
    }

    for(int i = 0; i < N; i++){
        if(pipe(&playerPipes[2*i]) < 0){
            ERR("pipe");
        }
    }

    pid_t pid;
    int winnerPID;
    int winnerBytes = 0;
    for(int i = 0; i < N; i++){
        //create children
        pid = fork();
        if(pid < 0) ERR("fork");
        if(pid == 0){
            //child runs
            
            // every child has pipe Pn->Pn+1
            // playerPipes[0] is between child 0 and 1
            // 0 sends 1 a card and reads from N
            //open read end of the prev pipe, write end of the next pipe
            int prev = (i-1+N)%N;
            for(int j = 0; j < N; j++){
                if(i == j){
                    // own
                    if(close(playerPipes[2*j]) < 0){
                        ERR("close");
                    }
                }
                else if(j == prev){

                    if(close(playerPipes[2*j+1])<0){
                        ERR("close");
                    }
                    
                }
                else{
    
                    if(close(playerPipes[2*j])<0){
                        ERR("close");
                    }
                    if(close(playerPipes[2*j+1])<0){
                        ERR("close");
                    }

                }
                
            }
        
            close(SHARED[0]);
            printf("[%d] Cards: ", getpid());
            playersList[i].pid = getpid();
            print_array(playersList[i].cards, M);
            sleep(1);
            child_work(playersList[i], playerPipes[2*i+1], playerPipes[2*prev], M, SHARED[1]);
            close(playerPipes[2*i+1]);
            if(i == 0){
                if(close(playerPipes[2*(N-1)])<0){
                    ERR("close");
                }
            }
            else{
                if(close(playerPipes[2*i])<0){
                    ERR("close");
                }
    
            }
            close(SHARED[1]);
            free(playerPipes);
            exit(0);
        }
        
    }
    close(SHARED[1]);
    if((winnerBytes = read(SHARED[0], &winnerPID, sizeof(int))) < 0){
        ERR("read");
    }
    if(winnerBytes){
        printf("[%d] won!\n", winnerPID);
        kill(0, SIGTERM);
    }
    for(int j = 0; j < 2*N; j++){
        
        if(close(playerPipes[j])<0){
            ERR("close");
        }

    }
    close(SHARED[0]);
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
    }

    create_players(N,M,playersList, deck);
    while(wait(NULL) > 0);
    for(int i=0;i<N;i++){
        free(playersList[i].cards);
    }
    free(playersList);
    printf("Open descriptors = %d\n", count_descriptors());
    return 0;
}