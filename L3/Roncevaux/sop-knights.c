#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>



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
} knight_t;

typedef struct {
    knight_t* FranciSoldiers;
    knight_t* SaraceniSoldiers;
    pthread_mutex_t* knightMutexes;
} shared_t;

int main(int argc, char* argv[])
{
    srand(getpid());
    FILE* francitxt, *saracenitxt;
    if((francitxt = fopen("franci.txt", "r")) == NULL)
    {
        printf("Franks have not arrived on the battlefield\n");
        ERR("fopen");
    }
    if((saracenitxt = fopen("saraceni.txt", "r")) == NULL)
    {
        printf("Saracens have not arrived on the battlefield\n");
        ERR("fopen");
    }

    int noFrancis, noSaracenis;
    fscanf(francitxt,"%d", &noFrancis);
    fscanf(saracenitxt,"%d", &noSaracenis);

    shared_t* sharedStock;
    if(MAP_FAILED == (sharedStock = (shared_t*)mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))){
        ERR("mmap");
    }

    if((sharedStock->FranciSoldiers = calloc(noFrancis, sizeof(knight_t))) == NULL)
        ERR("calloc");
    if((sharedStock->SaraceniSoldiers = calloc(noSaracenis, sizeof(knight_t))) == NULL)
        ERR("calloc");

    for (int i = 0; i < noFrancis; ++i){
        fscanf(francitxt,"%s %d %d", sharedStock->FranciSoldiers[i].name, &sharedStock->FranciSoldiers[i].HP, &sharedStock->FranciSoldiers[i].attack);
        printf("I am Frankish knight <%s>. I will serve my king with my <%d> HP and <%d> attack\n",sharedStock->FranciSoldiers[i].name, sharedStock->FranciSoldiers[i].HP, sharedStock->FranciSoldiers[i].attack);
    }
    for (int i = 0; i < noSaracenis; ++i){
        fscanf(saracenitxt,"%s %d %d", sharedStock->SaraceniSoldiers[i].name, &sharedStock->SaraceniSoldiers[i].HP, &sharedStock->SaraceniSoldiers[i].attack);
        printf("I am Spanish knight <%s>. I will serve my king with my <%d> HP and <%d> attack\n",sharedStock->SaraceniSoldiers[i].name, sharedStock->SaraceniSoldiers[i].HP, sharedStock->SaraceniSoldiers[i].attack);
    }

    fclose(francitxt);
    fclose(saracenitxt);
    free(sharedStock->FranciSoldiers);
    free(sharedStock->SaraceniSoldiers);
    munmap(sharedStock, sizeof(shared_t));
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}