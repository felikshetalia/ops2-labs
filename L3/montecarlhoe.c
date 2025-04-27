#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define MONTE_CARLO_ITERS 100000
#define LOG_LEN 8
#define SHM_NAME "/shum"

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


void usage(char* name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "10000 >= n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sigchld_handler(int sig, siginfo_t *s, void *p)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
    }
}

void child_work(int n, char* log, float* data)
{
    int sample = 0;
    srand(getpid());
    for(int i = 0; i < MONTE_CARLO_ITERS; i++){
        double x = ((double)rand()) / RAND_MAX;
        double y = ((double)rand()) / RAND_MAX;
        if((x*x + y*y) <= 1.0)
            sample++;
    }
    data[n] = ((float)sample) / MONTE_CARLO_ITERS;
    // this approximates to pi/4
    char buf[LOG_LEN + 1];
    // to format as string?
    snprintf(buf, LOG_LEN + 1, "%7.5f\n", data[n] * 4.0f);
    // data[n]*4.0f is pi
    memcpy(log + n * LOG_LEN, buf, LOG_LEN);

    // log takes the results as strings, data as float
}

void parent_work(int n, float* data)
{
    sethandler(sigchld_handler, SIGCHLD);
    float sum = 0.0;
    for(int i = 0; i < n; i++)
    {
        // taking avg of all the approximations
        sum += data[i];
    }
    float res = 4*(sum / n);
    printf("Pi is approximately %f\n", res);
}

void create_children(int n, float* data, char* log){
    while(n-- > 0){
        pid_t pid = fork();
        if(pid == 0)
        {
            // child
            child_work(n,log,data);
            exit(0);
        }
        if(pid < 0){
            ERR("fork");
        }
    }
}

int main(int argc, char** argv)
{
    printf("WELCOME TO MONTE CARLO PROGRAM.\n");
    int n = atoi(argv[1]);
    if (n < 0 || n > 100){
        usage(argv[0]);
    }

    // int log_fd = open("./log.txt", O_CREAT | O_RDWR | O_TRUNC, -1);
    // if(log_fd < 0){
    //     ERR("open");
    // }
    int shum_fd;
    if((shum_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_TRUNC, 0600)) < 0){
        ERR("shm_open");
    }
    if(ftruncate(shum_fd, n * LOG_LEN) < 0){
        ERR("ftruncate");
    }
    char *log = (char*)mmap(NULL, n*LOG_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, shum_fd, 0);
    if(log == MAP_FAILED){
        ERR("mmap");
    }
    if(close(shum_fd) < 0){
        ERR("close");
    }
    float *data;
    if((data = (float*)mmap(NULL, n * sizeof(float), PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
        ERR("mmap");

    create_children(n, data, log);
    while(wait(NULL) > 0);
    parent_work(n, data);
    
    if(munmap(data, n*sizeof(float))) ERR("munmap");
    if (msync(log, n * LOG_LEN, MS_SYNC)) ERR("msync");
    if(munmap(log, n*LOG_LEN)) ERR("munmap");
    shm_unlink(SHM_NAME);
    return EXIT_SUCCESS;
}