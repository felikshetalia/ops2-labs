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
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define BOSS_FIFO "/tmp/boss"

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

void parent_work(){

}
void factory_work(int fd, int writeEnd){
    char* buffer[20];
    char* msg = "I am speaking to items\n";
    int ret;
    if((ret = read()))
    int ret;
    if((ret == write(writeEnd, msg, sizeof(msg))) < 0){
        ERR("write");
    }
    else{
        printf("Message sent to item successfully.\n");
    }
    if(close(fd) < 0){
        ERR("close");
    }
    if(close(writeEnd) < 0){
        ERR("close");
    }
}
void employee_work(int fd, int writeEnd, int fifo){
    char* buffer[20];
    char* msg = "I am speaking to factory\n";
    int ret;
    if((ret == write(writeEnd, msg, sizeof(msg))) < 0){
        ERR("write");
    }
    else{
        printf("Message sent to factory successfully.\n");
    }
    if(close(fd) < 0){
        ERR("close");
    }
    if(close(writeEnd) < 0){
        ERR("close");
    }
}
void item_work(int fd, int writeEnd){
    char* buffer[20];
    char* msg = "I am speaking to the boss\n";
    int ret;
    if((ret == write(writeEnd, msg, sizeof(msg))) < 0){
        ERR("write");
    }
    else{
        printf("Message sent to boss successfully.\n");
    }
    if(close(fd) < 0){
        ERR("close");
    }
    if(close(writeEnd) < 0){
        ERR("close");
    }
}

void fork_children(int N, int M, int P, int fifo){
    int* EmployeesFD;
    int* FactoriesFD;
    int* ItemsFD;
    if((EmployeesFD = calloc(2*N, sizeof(int))) == NULL){
        ERR("calloc");
    }
    if((FactoriesFD = calloc(2*M, sizeof(int))) == NULL){
        ERR("calloc");
    }
    if((ItemsFD = calloc(2*P, sizeof(int))) == NULL){
        ERR("calloc");
    }
    for(int i = 0; i < N; i++){
        if(pipe(&EmployeesFD[2*i])<0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < M; i++){
        if(pipe(&FactoriesFD[2*i])<0){
            ERR("pipe");
        }
    }
    for(int i = 0; i < P; i++){
        if(pipe(&ItemsFD[2*i])<0){
            ERR("pipe");
        }
    }
    // for employees
    for(int i = 0; i < N; i++){
        pid_t pid = fork();
        if(pid < 0){
            ERR("fork");
        }
        if(pid == 0){
            // child running
            // close unused descriptors
            for(int j = 0; j < N; j++){
                if(i != j){
                    if(close(EmployeesFD[2*j]) < 0){
                        ERR("close"); // close other read ends
                    }
                }
                if(close(EmployeesFD[2*j+1]) < 0){
                    ERR("close"); // close all write ends
                }
            }
            for(int j = 0; j < M; j++){
                if(i != j){
                    
                    if(close(FactoriesFD[2*j+1]) < 0){
                        ERR("close"); // close other write ends
                    }
                }
                if(close(FactoriesFD[2*j]) < 0){
                    ERR("close"); // close other read ends
                }
            }
            for(int j = 0; j < P; j++){
                
                if(close(ItemsFD[2*j]) < 0){
                    ERR("close"); // close other read ends
                }
                if(close(ItemsFD[2*j+1]) < 0){
                    ERR("close"); // close other write ends
                }
            
            }
            employee_work(EmployeesFD[2*i], FactoriesFD[2*i+1], fifo);
            free(EmployeesFD);
            free(FactoriesFD);
            free(ItemsFD);
            exit(0);
        }
        
    }
    // factories
    for(int i = 0; i < M; i++){
        pid_t pid = fork();
        if(pid < 0){
            ERR("fork");
        }
        if(pid == 0){
            // child running
            // close unused descriptors
            for(int j = 0; j < N; j++){
                
                if(close(EmployeesFD[2*j]) < 0){
                    ERR("close"); // close other read ends
                }
                
                if(close(EmployeesFD[2*j+1]) < 0){
                    ERR("close"); // close all write ends
                }
            }
            for(int j = 0; j < M; j++){
                if(i != j){
                    if(close(FactoriesFD[2*j]) < 0){
                        ERR("close"); // close other read ends
                    } 
                }
                if(close(FactoriesFD[2*j+1]) < 0){
                    ERR("close"); // close other write ends
                }
            }
            for(int j = 0; j < P; j++){
                if(i != j){
                    if(close(ItemsFD[2*j+1]) < 0){
                        ERR("close"); // close other write ends
                    } 
                } 
                if(close(ItemsFD[2*j]) < 0){
                    ERR("close"); // close other read ends
                }    
            }
            factory_work(FactoriesFD[2*i], ItemsFD[2*i+1]);
            free(EmployeesFD);
            free(FactoriesFD);
            free(ItemsFD);
            exit(0);
        }
        
    }
    // for items
    for(int i = 0; i < P; i++){
        pid_t pid = fork();
        if(pid < 0){
            ERR("fork");
        }
        if(pid == 0){
            // child running
            // close unused descriptors
            for(int j = 0; j < N; j++){
                
                if(close(EmployeesFD[2*j]) < 0){
                    ERR("close"); // close other read ends
                }
                
                if(close(EmployeesFD[2*j+1]) < 0){
                    ERR("close"); // close all write ends
                }
            }
            for(int j = 0; j < M; j++){
                if(i != j){
                    if(close(FactoriesFD[2*j]) < 0){
                        ERR("close"); // close other read ends
                    } 
                }
                if(close(FactoriesFD[2*j+1]) < 0){
                    ERR("close"); // close other write ends
                }
            }
            for(int j = 0; j < P; j++){
                if(i != j){
                    if(close(ItemsFD[2*j+1]) < 0){
                        ERR("close"); // close other write ends
                    } 
                } 
                if(close(ItemsFD[2*j]) < 0){
                    ERR("close"); // close other read ends
                }    
            }
            item_work(ItemsFD[2*i],fifo);
            free(EmployeesFD);
            free(FactoriesFD);
            free(ItemsFD);
            exit(0);
        }
        
    }
}

int main(int argc, char** argv){
    unlink(BOSS_FIFO);
    if(argc != 4){
        printf("Employees - n\nFactories - m\nItems - p");
        exit(EXIT_FAILURE);
    }
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    int p = atoi(argv[3]);

    if(mkfifo(BOSS_FIFO, 0666) < 0){
        ERR("mkfifo");
    }
    int boss_fd;
    if((boss_fd = open(BOSS_FIFO, O_RDWR)) < 0){
        ERR("open fifo");
    }
    char request[30];
    printf("Enter request: ");
    scanf("%s", request);
    fork_children(n,m,p,boss_fd);
    // parent

    while (wait(NULL) > 0);
    printf("Opened descriptors: %d\n", count_descriptors());
    return 0;
}