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

#define MAX_KNIGHT_NAME_LENGTH 20
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

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

typedef struct Soldier{
    char name[MAX_KNIGHT_NAME_LENGTH];
    int HP;
    int attack;
}Soldier;

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

void child_work(Soldier knight, int read_end, int* enemies, int enemyCount){

    // set read_end to nonblocking mode
    srand(getpid());
    fcntl(read_end, F_SETFL, O_NONBLOCK); //set flag
    int isAlive = 1;
    int aliveEnemies = enemyCount;
    int res;
    char r;
    while(isAlive && aliveEnemies > 0){
        // taking damage
        while(isAlive && aliveEnemies > 0){
            if((res = read(read_end, &r, 1)) == 0){
                aliveEnemies = 0;
                break;
            }
            if(res < 0 && errno == EAGAIN){
                break;
            }
            if(res < 0){
                ERR("read");
            }
            if(res == 1){
                knight.HP -= r;
                // we take damage
                if(knight.HP < 0){
                    isAlive = 0;
                    printf("%s has died\n", knight.name);
                    //aliveEnemies--;
                }
            }
        }
        // dealing damage
        while(isAlive && aliveEnemies > 0){
            int random_enemy = rand() & enemyCount;
            char dmg = rand() % knight.attack;
            // char bcs we send one byte
            int res = write(enemies[2 * random_enemy + 1], &dmg, 1);
            if(res < 0){
                if(errno == EPIPE){
                    aliveEnemies--;
                    int tmp = enemies[2*random_enemy + 1];
                    enemies[2*random_enemy+1] = enemies[2*aliveEnemies+1];
                    enemies[2*aliveEnemies+1] = tmp;
                    continue;
                }
                else
                    ERR("write");
            }
            if(dmg == 0){
                printf("%s attacks his enemy, however he deflected\n", knight.name);
            }
            if(dmg >= 1 && dmg <= 5){
                printf("%s goes to strike, he hit right and well\n", knight.name);
            }
            if(dmg >= 6){
                printf("%s strikes powerful blow, the shield he breaks and inflicts a big wound\n", knight.name);
            }
            msleep(rand()%10+1);
            break;
        }
    }

    close(read_end);
    for(int i = 0; i < enemyCount; i++){
        if (close(enemies[2*i +1]) < 0){
            ERR("close");
        }
    }
}

void create_children(Soldier* franci, Soldier* saraceni, int franciNo, int saraceniNo){
    int* FranciFD;
    int* SaraceniFD;
    set_handler(SIG_IGN, SIGPIPE);
    if((FranciFD = calloc(franciNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    if((SaraceniFD = calloc(saraceniNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    for(int i = 0; i < franciNo; i++){
        if(pipe(&FranciFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo; i++){
        if(pipe(&SaraceniFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < franciNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < franciNo; j++){
                if(i != j){
                    if(close(FranciFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(FranciFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            for(int j = 0; j < saraceniNo; j++){ 
                if(close(SaraceniFD[2*j]) < 0){
                    ERR("close"); //all read ends
                } 
            }
            child_work(franci[i],FranciFD[2*i], SaraceniFD, saraceniNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    // now for spainards
    for(int i = 0; i < saraceniNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < saraceniNo; j++){
                if(i != j){
                    if(close(SaraceniFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(SaraceniFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            child_work(saraceni[i],SaraceniFD[2*i], FranciFD, franciNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    for(int i = 0; i < franciNo*2; i++){
        if(close(FranciFD[i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo*2; i++){
        if(close(SaraceniFD[i]) < 0){
            ERR("close"); //read ends except the current one
        }
    }
    free(SaraceniFD);
    free(FranciFD);
}

const char* franci_pth = "/mnt/c/Users/cansu/OneDrive/Masaüstü/ops stuff/L1/src/franci.txt";
const char* saraceni_pth = "/mnt/c/Users/cansu/OneDrive/Masaüstü/ops stuff/L1/src/saraceni.txt";

int main(int argc, char** argv){
    // stage 1
    FILE* franci_fp, *saraceni_fp;
    if((franci_fp = fopen(franci_pth, "r")) == NULL){
        printf("Franks have not arrived on the battlefield");
        ERR("fopen");
    }
    if((saraceni_fp = fopen(saraceni_pth, "r")) == NULL){
        printf("Saracens have not arrived on the battlefield");
        ERR("fopen");
    }
    int franciCount, saraceniCount;
    fscanf(franci_fp, "%d", &franciCount);
    fscanf(saraceni_fp, "%d", &saraceniCount);
    //printf("%d %d", franciCount, saraceniCount);
    Soldier *franci;
    Soldier *saraceni;
    if((franci = calloc(franciCount, sizeof(Soldier)))==NULL){
        ERR("malloc");
    }

    if((saraceni = calloc(saraceniCount, sizeof(Soldier)))==NULL){
        ERR("malloc");
    }

    for(int i=0;i<franciCount;i++){
        fscanf(franci_fp, "%s %d %d", franci[i].name, &franci[i].HP, &franci[i].attack);
        printf("I am Frankish knight %s. I will serve my king with my %d HP and %d attack\n",
        franci[i].name, franci[i].HP, franci[i].attack);
    }
    for(int i=0;i<saraceniCount;i++){
        fscanf(saraceni_fp, "%s %d %d", saraceni[i].name, &saraceni[i].HP, &saraceni[i].attack);
        printf("I am Spanish knight %s. I will serve my king with my %d HP and %d attack\n",
            saraceni[i].name, saraceni[i].HP, saraceni[i].attack);
    }
    create_children(franci, saraceni,franciCount,saraceniCount);
    while (wait(NULL) > 0);
    srand(time(NULL));
    printf("\n");
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}

// stage 1
/*
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

#define MAX_KNIGHT_LENGTH 20

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

typedef struct knight_t{
    char name[MAX_KNIGHT_NAME_LENGTH];
    int HP;
    int attack;
}

int main(int argc, char* argv[])
{
    FILE* franci_f = fopen("C:/Users/cansu/OneDrive/Masaüstü/ops stuff/L1/src/franci.txt", "r");
    FILE* saraceni_f = fopen("C:/Users/cansu/OneDrive/Masaüstü/ops stuff/L1/src/saraceni.txt", "r");
    if((franci_f == NULL ) || (saraceni_f == NULL)){
        printf("Franks/Saracens have not arrived on the battlefield\n");
        ERR("fopen");
    }
    int franciNo, saraceniNo;
    fscanf(franci_f, "%d", &franciNo);
    fscanf(saraceni_f, "%d", &saraceniNo);
    knight_t franci = calloc(franciNo, sizeof(knight_t));
    knight_t saraceni = calloc(saraceniNo, sizeof(knight_t));
    if((franci == NULL) || (saraceni == NULL)){
        ERR("calloc");
    }
    for(int i = 0; i < franciNo; i++){
        fscanf(franci_f, "%s %d %d", &franci[i].name, &franci[i].HP, &franci[i].attack);
        printf("I am Frankish knight %s. I will serve my king with my %d and %d attack\n", franci[i].name, franci[i].HP, franci[i].attack)
    }
    for(int i = 0; i < saraceniNo; i++){
        fscanf(saraceni_f, "%s %d %d", &saraceni[i].name, &saraceni[i].HP, &saraceni[i].attack);
        printf("I am Spanish knight %s. I will serve my king with my %d and %d attack\n", saraceni[i].name, saraceni[i].HP, saraceni[i].attack)
    }
    fclose(franci_f);
    fclose(saraceni_f);
    free(franci);
    free(saraceni);
    srand(time(NULL));
    printf("Opened descriptors: %d\n", count_descriptors());
}

*/

// stage 2

/*
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

#define MAX_KNIGHT_LENGTH 20

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

typedef struct knight_t{
    char name[MAX_KNIGHT_NAME_LENGTH];
    int HP;
    int attack;
} knight_t;


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

void child_work(knight_t franci, int read_end, int* enemies, int enemyCount){
    close(read_end);
    for(int i = 0; i < enemyCount; i++){
        if (close(enemies[2*i +1]) < 0){
            ERR("close");
        }
    }
}

void create_pipes_and_fork(knight_t* franci, knight_t* saraceni, int franciNo, int saraceniNo){
    int* FranciFD;
    int* SaraceniFD;
    if((FranciFD = calloc(franciNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    if((SaraceniFD = calloc(saraceniNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    for(int i = 0; i < franciNo; i++){
        if(pipe(&FranciFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo; i++){
        if(pipe(&SaraceniFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < franciNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < franciNo; j++){
                if(i != j){
                    if(close(FranciFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(FranciFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            for(int j = 0; j < saraceniNo; j++){ 
                if(close(SaraceniFD[2*j]) < 0){
                    ERR("close"); //all read ends
                } 
            }
            child_work(franci[i],FranciFD[2*i], SaraceniFD, saraceniNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    // now for spainards
    for(int i = 0; i < saraceniNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < saraceniNo; j++){
                if(i != j){
                    if(close(SaraceniFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(SaraceniFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            child_work(saraceni[i],SaraceniFD[2*i], FranciFD, franciNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    for(int i = 0; i < franciNo*2; i++){
        if(close(FranciFD[i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo*2; i++){
        if(close(SaraceniFD[i]) < 0){
            ERR("close"); //read ends except the current one
        }
    }
    free(SaraceniFD);
    free(FranciFD);
}

int main(int argc, char* argv[])
{
    FILE* franci_f;
    FILE* saraceni_f;

    if ((franci_f = fopen("franci.txt", "r")) == NULL)
    {
        printf("Franks have not arrived on the battlefield");
        ERR("fopen");
    }
    if ((saraceni_f = fopen("saraceni.txt", "r")) == NULL)
    {
        printf("Saraceni have not arrived on the battlefield");
        ERR("fopen");
    }
    int franciNo, saraceniNo;
    fscanf(franci_f, "%d", &franciNo);
    fscanf(saraceni_f, "%d", &saraceniNo);
    knight_t* franci;
    if ((franci = calloc(franciNo, sizeof(knight_t))) == NULL)
        ERR("calloc");
    knight_t* saraceni;
    if ((saraceni = calloc(saraceniNo, sizeof(knight_t))) == NULL)
        ERR("calloc");
    for(int i = 0; i < franciNo; i++){
        fscanf(franci_f, "%s %d %d", franci[i].name, &franci[i].HP, &franci[i].attack);
        //printf("I am Frankish knight %s. I will serve my king with my %d and %d attack\n", franci[i].name, franci[i].HP, franci[i].attack);
    }
    for(int i = 0; i < saraceniNo; i++){
        fscanf(saraceni_f, "%s %d %d", saraceni[i].name, &saraceni[i].HP, &saraceni[i].attack);
        //printf("I am Spanish knight %s. I will serve my king with my %d and %d attack\n", saraceni[i].name, saraceni[i].HP, saraceni[i].attack);
    }
    fclose(franci_f);
    fclose(saraceni_f);

    create_pipes_and_fork(franci, saraceni, franciNo, saraceniNo);
    while (wait(NULL) > 0);
    
    free(franci);
    free(saraceni);
    srand(time(NULL));
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}

*/

// stage 3

/*
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

#define MAX_KNIGHT_LENGTH 20

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

typedef struct knight_t{
    char name[MAX_KNIGHT_NAME_LENGTH];
    int HP;
    int attack;
} knight_t;


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

void child_work(knight_t knight, int read_end, int* enemies, int enemyCount){

    // set read_end to nonblocking mode
    srand(getpid());
    fcntl(read_end, F_SETFL, O_NONBLOCK); //set flag
    int res;
    char r;
    while(1){
        // taking damage
        while(1){
            if((res = read(read_end, &r, 1)) == 0){
                break;
            }
            if(res < 0 && errno == EAGAIN){
                break;
            }
            if(res < 0){
                ERR("read");
            }
            if(res == 1){
                knight.HP -= r;
                // we take damage
            }
        }
        // dealing damage
        while(1){
            int random_enemy = rand() & enemyCount;
            char dmg = rand() % knight.attack;
            // char bcs we send one byte
            int res = write(enemies[2 * random_enemy + 1], &dmg, 1);
            if(res < 0){
                ERR("write");
            }
            if(dmg == 0){
                printf("%s attacks his enemy, however he deflected\n", knight.name);
            }
            if(dmg >= 1 && dmg <= 5){
                printf("%s goes to strike, he hit right and well\n", knight.name);
            }
            if(dmg >= 6){
                printf("%s strikes powerful blow, the shield he breaks and inflicts a big wound\n", knight.name);
            }
            msleep(rand()%10+1);
        }
    }

    close(read_end);
    for(int i = 0; i < enemyCount; i++){
        if (close(enemies[2*i +1]) < 0){
            ERR("close");
        }
    }
}

void create_pipes_and_fork(knight_t* franci, knight_t* saraceni, int franciNo, int saraceniNo){
    int* FranciFD;
    int* SaraceniFD;
    if((FranciFD = calloc(franciNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    if((SaraceniFD = calloc(saraceniNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    for(int i = 0; i < franciNo; i++){
        if(pipe(&FranciFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo; i++){
        if(pipe(&SaraceniFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < franciNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < franciNo; j++){
                if(i != j){
                    if(close(FranciFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(FranciFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            for(int j = 0; j < saraceniNo; j++){ 
                if(close(SaraceniFD[2*j]) < 0){
                    ERR("close"); //all read ends
                } 
            }
            child_work(franci[i],FranciFD[2*i], SaraceniFD, saraceniNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    // now for spainards
    for(int i = 0; i < saraceniNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < saraceniNo; j++){
                if(i != j){
                    if(close(SaraceniFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(SaraceniFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            child_work(saraceni[i],SaraceniFD[2*i], FranciFD, franciNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    for(int i = 0; i < franciNo*2; i++){
        if(close(FranciFD[i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo*2; i++){
        if(close(SaraceniFD[i]) < 0){
            ERR("close"); //read ends except the current one
        }
    }
    free(SaraceniFD);
    free(FranciFD);
}

int main(int argc, char* argv[])
{
    FILE* franci_f;
    FILE* saraceni_f;

    if ((franci_f = fopen("franci.txt", "r")) == NULL)
    {
        printf("Franks have not arrived on the battlefield");
        ERR("fopen");
    }
    if ((saraceni_f = fopen("saraceni.txt", "r")) == NULL)
    {
        printf("Saraceni have not arrived on the battlefield");
        ERR("fopen");
    }
    int franciNo, saraceniNo;
    fscanf(franci_f, "%d", &franciNo);
    fscanf(saraceni_f, "%d", &saraceniNo);
    knight_t* franci;
    if ((franci = calloc(franciNo, sizeof(knight_t))) == NULL)
        ERR("calloc");
    knight_t* saraceni;
    if ((saraceni = calloc(saraceniNo, sizeof(knight_t))) == NULL)
        ERR("calloc");
    for(int i = 0; i < franciNo; i++){
        fscanf(franci_f, "%s %d %d", franci[i].name, &franci[i].HP, &franci[i].attack);
        //printf("I am Frankish knight %s. I will serve my king with my %d and %d attack\n", franci[i].name, franci[i].HP, franci[i].attack);
    }
    for(int i = 0; i < saraceniNo; i++){
        fscanf(saraceni_f, "%s %d %d", saraceni[i].name, &saraceni[i].HP, &saraceni[i].attack);
        //printf("I am Spanish knight %s. I will serve my king with my %d and %d attack\n", saraceni[i].name, saraceni[i].HP, saraceni[i].attack);
    }
    fclose(franci_f);
    fclose(saraceni_f);

    create_pipes_and_fork(franci, saraceni, franciNo, saraceniNo);
    while (wait(NULL) > 0);
    
    free(franci);
    free(saraceni);
    srand(time(NULL));
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}

*/

/*

//stage 4

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

#define MAX_KNIGHT_LENGTH 20

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

typedef struct knight_t{
    char name[MAX_KNIGHT_NAME_LENGTH];
    int HP;
    int attack;
} knight_t;


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

void child_work(knight_t knight, int read_end, int* enemies, int enemyCount){

    // set read_end to nonblocking mode
    srand(getpid());
    int alive_enemies = enemyCount;
    fcntl(read_end, F_SETFL, O_NONBLOCK); //set flag
    int res;
    char r;
    int alive = 1;
    while(alive && alive_enemies > 0){
        // taking damage
        while(alive && alive_enemies > 0){
            if((res = read(read_end, &r, 1)) == 0){
                alive_enemies = 0;
                break;
            }
            if(res < 0 && errno == EAGAIN){
                break;
            }
            else if(res < 0){
                ERR("read");
            }
            if(res == 1){
                knight.HP -= r;
                // we take damage
                if(knight.HP < 0){
                    printf("%s dies\n", knight.name);
                    alive = 0;
                }
            }
        }
        // dealing damage
        while(alive && alive_enemies > 0){
            int random_enemy = rand() & enemyCount;
            char dmg = rand() % knight.attack;
            // char bcs we send one byte
            int res = write(enemies[2 * random_enemy + 1], &dmg, 1);
            if(res < 0){
                if(errno == EPIPE){
                    alive_enemies--;
                    int tmp = enemies[2*random_enemy + 1];
                    enemies[2*random_enemy+1] = enemies[2*alive_enemies+1];
                    enemies[2*alive_enemies+1] = tmp;
                    continue;
                }
                else
                    ERR("write");
            }
            if(dmg == 0){
                printf("%s attacks his enemy, however he deflected\n", knight.name);
            }
            if(dmg >= 1 && dmg <= 5){
                printf("%s goes to strike, he hit right and well\n", knight.name);
            }
            if(dmg >= 6){
                printf("%s strikes powerful blow, the shield he breaks and inflicts a big wound\n", knight.name);
            }
            msleep(rand()%10+1);
            break;
        }
    }

    close(read_end);
    for(int i = 0; i < enemyCount; i++){
        if (close(enemies[2*i +1]) < 0){
            ERR("close");
        }
    }
}

void create_pipes_and_fork(knight_t* franci, knight_t* saraceni, int franciNo, int saraceniNo){
    int* FranciFD;
    int* SaraceniFD;
    set_handler(SIG_IGN, SIGPIPE);
    if((FranciFD = calloc(franciNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    if((SaraceniFD = calloc(saraceniNo*2, sizeof(int))) == NULL){
        ERR("calloc");
    }
    for(int i = 0; i < franciNo; i++){
        if(pipe(&FranciFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo; i++){
        if(pipe(&SaraceniFD[2*i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < franciNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < franciNo; j++){
                if(i != j){
                    if(close(FranciFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(FranciFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            for(int j = 0; j < saraceniNo; j++){ 
                if(close(SaraceniFD[2*j]) < 0){
                    ERR("close"); //all read ends
                } 
            }
            child_work(franci[i],FranciFD[2*i], SaraceniFD, saraceniNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    // now for spainards
    for(int i = 0; i < saraceniNo; i++){
        pid_t pid = fork();
        if(pid == 0){
            for(int j = 0; j < saraceniNo; j++){
                if(i != j){
                    if(close(SaraceniFD[2*j]) < 0){
                        ERR("close"); //read ends except the current one
                    }
                }
                if(close(SaraceniFD[2*j+1]) < 0){
                    ERR("close"); //write ends
                }
            }
            child_work(saraceni[i],SaraceniFD[2*i], FranciFD, franciNo);
            free(franci);
            free(saraceni);
            free(SaraceniFD);
            free(FranciFD);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
    for(int i = 0; i < franciNo*2; i++){
        if(close(FranciFD[i]) < 0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < saraceniNo*2; i++){
        if(close(SaraceniFD[i]) < 0){
            ERR("close"); //read ends except the current one
        }
    }
    free(SaraceniFD);
    free(FranciFD);
}

int main(int argc, char* argv[])
{
    FILE* franci_f;
    FILE* saraceni_f;

    if ((franci_f = fopen("franci.txt", "r")) == NULL)
    {
        printf("Franks have not arrived on the battlefield");
        ERR("fopen");
    }
    if ((saraceni_f = fopen("saraceni.txt", "r")) == NULL)
    {
        printf("Saraceni have not arrived on the battlefield");
        ERR("fopen");
    }
    int franciNo, saraceniNo;
    fscanf(franci_f, "%d", &franciNo);
    fscanf(saraceni_f, "%d", &saraceniNo);
    knight_t* franci;
    if ((franci = calloc(franciNo, sizeof(knight_t))) == NULL)
        ERR("calloc");
    knight_t* saraceni;
    if ((saraceni = calloc(saraceniNo, sizeof(knight_t))) == NULL)
        ERR("calloc");
    for(int i = 0; i < franciNo; i++){
        fscanf(franci_f, "%s %d %d", franci[i].name, &franci[i].HP, &franci[i].attack);
        //printf("I am Frankish knight %s. I will serve my king with my %d and %d attack\n", franci[i].name, franci[i].HP, franci[i].attack);
    }
    for(int i = 0; i < saraceniNo; i++){
        fscanf(saraceni_f, "%s %d %d", saraceni[i].name, &saraceni[i].HP, &saraceni[i].attack);
        //printf("I am Spanish knight %s. I will serve my king with my %d and %d attack\n", saraceni[i].name, saraceni[i].HP, saraceni[i].attack);
    }
    fclose(franci_f);
    fclose(saraceni_f);

    create_pipes_and_fork(franci, saraceni, franciNo, saraceniNo);
    while (wait(NULL) > 0);
    
    free(franci);
    free(saraceni);
    srand(time(NULL));
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}


DETAILS ABT THURSDAY:
https://github.com/michau-s/OPS2-L1-Consultations/blob/master/sop2-5/src/sop-roncevaux.c
pipes and fifos

*/
