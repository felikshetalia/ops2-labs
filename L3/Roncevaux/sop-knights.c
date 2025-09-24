#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
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
    int NumberFrancis;
    int NumberSaracenis;
    knight_t* FranciSoldiers;
    knight_t* SaraceniSoldiers;
    pthread_mutex_t* franciMX;
    pthread_mutex_t* saraceniMX;
} shared_t;

pthread_mutexattr_t init_mutex(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

    return attr;
}

void mutex_lock_handler(pthread_mutex_t* mutex){
    int error;
    if ((error = pthread_mutex_lock(mutex)) != 0)
    {
        if (error == EOWNERDEAD)
        {
            pthread_mutex_consistent(mutex);
        }
        else
        {
            ERR("pthread_mutex_lock");
        }
    }
}

void* franci_work(void* args){
    shared_t* block = (shared_t*) args;
    while(1){
        int randomEnemy = rand() % (block->NumberSaracenis + 1);
        int randAttack = rand() % (block->FranciSoldiers)
        mutex_lock_handler(&block->franciMX[0]);
        // printf("I am Frankish knight <%s>. I will serve my king with my <%d> HP and <%d> attack\n",block->FranciSoldiers[i].name, block->FranciSoldiers[i].HP, block->FranciSoldiers[i].attack);
        if(block->SaraceniSoldiers[randomEnemy].HP){

        }
        pthread_mutex_unlock(&block->franciMX[0]);

    }    

    return NULL;
}

void* saraceni_work(void* args){
    shared_t* block = (shared_t*) args;

    while(1){
        mutex_lock_handler(&block->saraceniMX[0]);
        // printf("I am Saraceni knight <%s>. I will serve my king with my <%d> HP and <%d> attack\n",block->SaraceniSoldiers[i].name, block->SaraceniSoldiers[i].HP, block->SaraceniSoldiers[i].attack);
        pthread_mutex_unlock(&block->saraceniMX[0]);

    }


    return NULL;
}


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
    sharedStock->NumberFrancis = noFrancis;
    sharedStock->NumberSaracenis = noSaracenis;
    if((sharedStock->FranciSoldiers = calloc(noFrancis, sizeof(knight_t))) == NULL)
        ERR("calloc");
    if((sharedStock->SaraceniSoldiers = calloc(noSaracenis, sizeof(knight_t))) == NULL)
        ERR("calloc");
    if((sharedStock->franciMX = calloc(noFrancis, sizeof(pthread_mutex_t))) == NULL)
        ERR("calloc");
    if((sharedStock->saraceniMX = calloc(noSaracenis, sizeof(pthread_mutex_t))) == NULL)
        ERR("calloc");

    for (int i = 0; i < noFrancis; ++i){
        fscanf(francitxt,"%s %d %d", sharedStock->FranciSoldiers[i].name, &sharedStock->FranciSoldiers[i].HP, &sharedStock->FranciSoldiers[i].attack);
    }
    for (int i = 0; i < noSaracenis; ++i){
        fscanf(saracenitxt,"%s %d %d", sharedStock->SaraceniSoldiers[i].name, &sharedStock->SaraceniSoldiers[i].HP, &sharedStock->SaraceniSoldiers[i].attack);
    }

    fclose(francitxt);
    fclose(saracenitxt);
    // init mutexes
    pthread_mutexattr_t mutexAttr = init_mutex();
    for (int i = 0; i < noFrancis; ++i){
        pthread_mutex_init(&sharedStock->franciMX[i], &mutexAttr);
    }
    for (int i = 0; i < noSaracenis; ++i){
        pthread_mutex_init(&sharedStock->saraceniMX[i], &mutexAttr);
    }

    // register threads
    pthread_t franciThreads[noFrancis];
    for (int i = 0; i < noFrancis; ++i){
        int ret;
        if ((ret = pthread_create(&franciThreads[i], NULL, franci_work, sharedStock)) != 0) {
            fprintf(stderr, "pthread_create(): %s", strerror(ret));
            return 1;
        }
        pthread_join(franciThreads[i], NULL);

    }
    pthread_t saraceniThreads[noSaracenis];
    for (int i = 0; i < noSaracenis; ++i){
        int ret;
        if ((ret = pthread_create(&saraceniThreads[i], NULL, saraceni_work, sharedStock)) != 0) {
            fprintf(stderr, "pthread_create(): %s", strerror(ret));
            return 1;
        }
        pthread_join(saraceniThreads[i], NULL);
    }

    // cleanup
    for (int i = 0; i < noFrancis; ++i){
        pthread_mutex_destroy(&sharedStock->franciMX[i]);
    }
    for (int i = 0; i < noSaracenis; ++i){
        pthread_mutex_destroy(&sharedStock->saraceniMX[i]);
    }
    pthread_mutexattr_destroy(&mutexAttr);

    free(sharedStock->FranciSoldiers);
    free(sharedStock->SaraceniSoldiers);
    free(sharedStock->franciMX);
    free(sharedStock->saraceniMX);
    munmap(sharedStock, sizeof(shared_t));
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}

// add near your includes
// typedef enum { TEAM_FRANK, TEAM_SARACEN } team_t;

// typedef struct {
//     shared_t *S;   // shared data
//     team_t team;   // which team
//     int idx;       // which knight within that team
// } thread_arg_t;

// static pthread_mutex_t print_mx = PTHREAD_MUTEX_INITIALIZER;

// pthread_mutexattr_t init_mutex(){
//     pthread_mutexattr_t attr;
//     pthread_mutexattr_init(&attr);
//     // You don't need robust or process-shared for thread-only use:
//     // pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
//     return attr;
// }
// void* knight_intro(void* args){
//     thread_arg_t* a = (thread_arg_t*)args;

//     pthread_mutex_lock(&print_mx);
//     if (a->team == TEAM_FRANK) {
//         knight_t *k = &a->S->FranciSoldiers[a->idx];
//         printf("I am Frankish knight <%s>. I will serve my king with my <%d> HP and <%d> attack\n",
//                k->name, k->HP, k->attack);
//     } else {
//         knight_t *k = &a->S->SaraceniSoldiers[a->idx];
//         printf("I am Saraceni knight <%s>. I will serve my king with my <%d> HP and <%d> attack\n",
//                k->name, k->HP, k->attack);
//     }
//     pthread_mutex_unlock(&print_mx);

//     return NULL;
// }

// after opening files:
// if (fscanf(francitxt, "%d", &noFrancis) != 1) ERR("fscanf");
// if (fscanf(saracenitxt, "%d", &noSaracenis) != 1) ERR("fscanf");

// // when reading knights:
// for (int i = 0; i < noFrancis; ++i){
//     if (fscanf(francitxt, "%19s %d %d",
//                sharedStock->FranciSoldiers[i].name,
//                &sharedStock->FranciSoldiers[i].HP,
//                &sharedStock->FranciSoldiers[i].attack) != 3) ERR("fscanf");
// }
// for (int i = 0; i < noSaracenis; ++i){
//     if (fscanf(saracenitxt, "%19s %d %d",
//                sharedStock->SaraceniSoldiers[i].name,
//                &sharedStock->SaraceniSoldiers[i].HP,
//                &sharedStock->SaraceniSoldiers[i].attack) != 3) ERR("fscanf");
// }
// allocate arg arrays so each thread has stable args
// thread_arg_t *fargs = calloc(noFrancis, sizeof(*fargs));
// thread_arg_t *sargs = calloc(noSaracenis, sizeof(*sargs));
// if (!fargs || !sargs) ERR("calloc");

// pthread_t *franciThreads   = calloc(noFrancis, sizeof(*franciThreads));
// pthread_t *saraceniThreads = calloc(noSaracenis, sizeof(*saraceniThreads));
// if (!franciThreads || !saraceniThreads) ERR("calloc");

// // create all Frankish threads
// for (int i = 0; i < noFrancis; ++i){
//     fargs[i].S = sharedStock;
//     fargs[i].team = TEAM_FRANK;
//     fargs[i].idx = i;
//     int ret = pthread_create(&franciThreads[i], NULL, knight_intro, &fargs[i]);
//     if (ret) { fprintf(stderr, "pthread_create(): %s\n", strerror(ret)); exit(1); }
// }

// // create all Saraceni threads
// for (int i = 0; i < noSaracenis; ++i){
//     sargs[i].S = sharedStock;
//     sargs[i].team = TEAM_SARACEN;
//     sargs[i].idx = i;
//     int ret = pthread_create(&saraceniThreads[i], NULL, knight_intro, &sargs[i]);
//     if (ret) { fprintf(stderr, "pthread_create(): %s\n", strerror(ret)); exit(1); }
// }

// // join everyone
// for (int i = 0; i < noFrancis; ++i)    pthread_join(franciThreads[i], NULL);
// for (int i = 0; i < noSaracenis; ++i)  pthread_join(saraceniThreads[i], NULL);
