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
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_KNIGHT_NAME_LENGTH 20

int set_handler(void (*f)(int), int sig)
{
    struct sigaction act = {0};
    act.sa_handler = f;
    if (sigaction(sig, &act, NULL) == -1)
        return -1;
    return 0;
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

typedef struct{
    char name[MAX_KNIGHT_NAME_LENGTH];
    int HP;
    int attack;
}knight_t;

// void child_work(int* FranciPipes, int* SaraceniPipes, int fNo, int sNo){

// }

void create_knights_and_pipes(int fNo, int sNo, knight_t* FrancisArray, knight_t* SaracenisArray){
    int* FranciPipes, *SaraceniPipes;
    if((FranciPipes = calloc(fNo*2, sizeof(int))) < 0){
        ERR("calloc");
    }
    if((SaraceniPipes = calloc(sNo*2, sizeof(int))) < 0){
        ERR("calloc");
    }
    for(int i = 0; i < fNo; i++){
        if(pipe(&FranciPipes[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < sNo; i++){
        if(pipe(&SaraceniPipes[2*i]) < 0){
            ERR("pipe");
        }
    }
    srand(getpid());
    pid_t pid;

    for(int i = 0; i < fNo; i++){
        //create children
        pid = fork();
        if(pid < 0) ERR("fork");
        if(pid == 0){
            //child runs
            for(int j = 0; j < fNo; j++){
                // close unused pipes
                if(i != j){
                    if(close(FranciPipes[2*j]) < 0){
                        ERR("close");
                    }
                }
                // close own write ends
                if(close(FranciPipes[2*j+1]) < 0){
                    ERR("close");
                }
            }
            for(int j = 0; j < sNo; j++){
                // close all read ends of the opponent
                if(close(SaraceniPipes[2*j]) < 0){
                    ERR("close");
                }
            }
            printf("I am Frankish knight %s. I will serve my king with my %d HP and %d attack\n",FrancisArray[i].name, FrancisArray[i].HP, FrancisArray[i].attack);
            free(FranciPipes);
            free(SaraceniPipes);
            free(FrancisArray);
            free(SaracenisArray);
            exit(0);
        }
    }
    for(int i = 0; i < sNo; i++){
        //create children
        pid = fork();
        if(pid < 0) ERR("fork");
        if(pid == 0){
            //child runs
            for(int j = 0; j < sNo; j++){
                // close unused pipes
                if(i != j){
                    if(close(SaraceniPipes[2*j]) < 0){
                        ERR("close");
                    }
                }
                // close own write ends
                if(close(SaraceniPipes[2*j+1]) < 0){
                    ERR("close");
                }
            }
            for(int j = 0; j < fNo; j++){
                // close all read ends of the opponent
                if(close(FranciPipes[2*j]) < 0){
                    ERR("close");
                }
            }
            printf("I am Spanish knight %s. I will serve my king with my %d HP and %d attack\n",SaracenisArray[i].name, SaracenisArray[i].HP, SaracenisArray[i].attack);
            
            free(FranciPipes);
            free(SaraceniPipes);
            free(FrancisArray);
            free(SaracenisArray);
            exit(0);
        }
        
    }
    
    for(int i = 0; i < fNo*2; i++){
        if(close(FranciPipes[i]) < 0){
            ERR("close");
        }
    }
    for(int i = 0; i < sNo*2; i++){
        if(close(SaraceniPipes[i]) < 0){
            ERR("close");
        }
    }
    free(FranciPipes);
    free(SaraceniPipes);

}

int main(int argc, char* argv[])
{
    srand(time(NULL));
    FILE* franciFD, *saraceniFD;
    if((franciFD = fopen("franci.txt", "r")) == NULL){
        printf("Franks have not arrived on the battlefield\n");
        ERR("fopen");
    }
    if((saraceniFD = fopen("saraceni.txt", "r")) == NULL){
        printf("Saracens have not arrived on the battlefield\n");
        ERR("fopen");
    }

    int numFrancis, numSaracenis;
    fscanf(franciFD, "%d", &numFrancis);
    fscanf(saraceniFD, "%d", &numSaracenis);

    printf("%d %d\n", numFrancis, numSaracenis);

    knight_t* FrancisArray, *SaracenisArray;

    if((FrancisArray = calloc(numFrancis, sizeof(knight_t))) < 0){
        ERR("calloc");
    }
    if((SaracenisArray = calloc(numSaracenis, sizeof(knight_t))) < 0){
        ERR("calloc");
    }

    for(int i = 0; i < numFrancis; i++){
        fscanf(franciFD, "%s %d %d", FrancisArray[i].name, &FrancisArray[i].HP, &FrancisArray[i].attack);
    }
    for(int i = 0; i < numSaracenis; i++){
        fscanf(saraceniFD, "%s %d %d", SaracenisArray[i].name, &SaracenisArray[i].HP, &SaracenisArray[i].attack);
    }

    create_knights_and_pipes(numFrancis, numSaracenis, FrancisArray, SaracenisArray);
    while(wait(NULL) > 0);
    free(FrancisArray);
    free(SaracenisArray);
    fclose(franciFD);
    fclose(saraceniFD);
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}