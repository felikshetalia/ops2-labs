#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILENAME "./dossier.txt"
#define MSG_LEN 128

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


volatile sig_atomic_t failed = 0;
void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sigchld_handler(int sig)
{
    int status;
    pid_t pid = waitpid(-1, &status, 0);

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGABRT) {
            printf("Child %d terminated unexpectedly with SIGABRT\n", pid);
        } else {
            printf("Child %d was killed by signal %d\n", pid, sig);
        }
    } else if (WIFEXITED(status)) {
        printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
    }
}

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "10000 >= n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

void count_occurences(char* str, char* shared, int idx){
    int dump[128] = {0};
    int i = 0;
    do{
        unsigned char ch = str[i];
        dump[ch]++;
        i++;
    }while(str[i] != '\0');

    for(int i=0;i<128;i++){
        // if(dump[i] > 0){
        //     printf("'%c' → %d times\n", i, dump[i]);
        // }
        if(dump[i]>0){
            snprintf(shared + (idx * 128 + i) * MSG_LEN, MSG_LEN,"'%c' → %d times\n", i, dump[i]);
        }
        else{
            shared[(idx * 128 + i) * MSG_LEN] = '\0'; // clean unused slot
        }
    }
    if(msync(shared, 128*idx*MSG_LEN, MS_SYNC) < 0){
        ERR("msync");
    }
}

void child_work(char* shared, int i){
    int fd;
    if((fd = open(FILENAME, O_CREAT | O_RDWR, -1)) < 0){
        ERR("open");
    }

    char* data;
    if(MAP_FAILED == (data = (char*)mmap(NULL, MSG_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0))){
        ERR("mmap");
    }
    
    printf("[%d] opened file\n", getpid());
    close(fd);
    if (rand() % 100 < 30) {
        printf("[%d] Oops, a child resigned\n");
        abort();  // Will raise SIGABRT
    }
    printf("%s\n", data);
    count_occurences(data, shared, i);
    munmap(data, MSG_LEN);
}

void pop_children(int n, char* shared){
    srand(getpid());
    pid_t pid;
    for(int i=0;i<n;i++){
        pid = fork();
        if(pid == 0){
            //printf("[%d] Child entered\n", getpid());
            child_work(shared, i);
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }

    }
}

int main(int argc, char** argv)
{
    if (argc != 2) usage(argv[0]);
    int n = atoi(argv[1]);

    char* shared;
    if(MAP_FAILED == (shared = (char*)mmap(NULL, 128*n*MSG_LEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))){
        ERR("mmap");
    }
    sethandler(sigchld_handler, SIGCHLD);

    pop_children(n, shared);

    while(wait(NULL) > 0);


    for(int i = 0; i < n * 128; i++) {
        if (shared[i * MSG_LEN] != '\0') {
            printf("%s", &shared[i * MSG_LEN]);
        }
    }
    
    munmap(shared, 128*n*MSG_LEN);

    return EXIT_SUCCESS;
}