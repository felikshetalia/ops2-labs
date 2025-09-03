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

void create_players(int N, int M, player_t* playersList, int* deck){
    srand(getpid());

    int *playerPipes;
    if((playerPipes = calloc(2*N, sizeof(int))) == NULL){
        ERR("calloc");
    }

    for(int i = 0; i < N; i++){
        if(pipe(&playerPipes[2*i]) < 0){
            ERR("pipe");
        }
    }

    pid_t pid;

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

            for(int j = 0; j < N; j++){
                if(i != j){
                    if(close(playerPipes[2*j+1]) < 0){
                        ERR("close");
                    }
                }
                if(i == 0){
                    if(j < N-1){
                        if(close(playerPipes[2*j])<0){
                            ERR("close");
                        }
                    }
                }
                else{
                    if(i!=j){
                        if(close(playerPipes[2*j])<0){
                            ERR("close");
                        }
                    }
                }
            }

            printf("[%d] Cards: ", getpid());
            playersList[i].pid = getpid();
            print_array(playersList[i].cards, M);

            free(playerPipes);
            exit(0);
        }
    }
    for(int j = 0; j < 2*N; j++){
        
        if(close(playerPipes[j])<0){
            ERR("close");
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