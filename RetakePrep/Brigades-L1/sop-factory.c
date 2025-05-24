#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))
#endif

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

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

void usage(char* program)
{
    printf("%s w1 w2 w3\n", program);
    exit(EXIT_FAILURE);
}

void first_brigade_work(int production_pipe_write, int boss_pipe)
{
    srand(getpid());

    printf("Worker %d from the first brigade: descriptors: %d\n", getpid(), count_descriptors());
}
void second_brigade_work(int production_pipe_write, int production_pipe_read, int boss_pipe)
{
    srand(getpid());


    printf("Worker %d from the second brigade: descriptors: %d\n", getpid(), count_descriptors());
}
void third_brigade_work(int production_pipe_read, int boss_pipe)
{
    srand(getpid());

    printf("Worker %d from the third brigade: descriptors: %d\n", getpid(), count_descriptors());
}

void create_workers(int w1, int w2, int w3)
{
    srand(getpid());
    int *w1_pipes, *w2_pipes, *w3_pipes;
    int FROM_1_TO_2[2], FROM_2_TO_3[2];

    if((w1_pipes = calloc(2*w1, sizeof(int))) == NULL)
        ERR("calloc");
    if((w2_pipes = calloc(2*w2, sizeof(int))) == NULL)
        ERR("calloc");
    if((w3_pipes = calloc(2*w3, sizeof(int))) == NULL)
        ERR("calloc");

    // initialize the pipes
    for(int i = 0; i < w1; i++)
    {
        if(pipe(&w1_pipes[2*i]) < 0)
            ERR("pipe");
    }
    for(int i = 0; i < w2; i++)
    {
        if(pipe(&w2_pipes[2*i]) < 0)
            ERR("pipe");
    }
    for(int i = 0; i < w3; i++)
    {
        if(pipe(&w3_pipes[2*i]) < 0)
            ERR("pipe");
    }

    if(pipe(FROM_1_TO_2) < 0)
        ERR("pipe");
    if(pipe(FROM_2_TO_3) < 0)
        ERR("pipe");

    pid_t pid;
    for(int i = 0; i < w1; i++)
    {
        pid = fork();

        if(pid < 0)
            ERR("fork");
        if(pid == 0)
        {
            //child
            // step 1 close unused pipes;
            

            // workers only write to the boss so close the reading ends
            for(int j = 0; j < w1; j++)
            {
                if(close(w1_pipes[2*j]) < 0)
                    ERR("close");
                if(i != j){
                    if(close(w1_pipes[2*j + 1]) < 0)
                        ERR("close");
                    // close writing end except your own
                }
            }
            for(int j = 0; j < w2; j++)
            {
                if(close(w2_pipes[2*j]) < 0)
                    ERR("close");
                
                if(close(w2_pipes[2*j + 1]) < 0)
                    ERR("close");
                
            }
            for(int j = 0; j < w3; j++)
            {
                if(close(w3_pipes[2*j]) < 0)
                    ERR("close");
                
                if(close(w3_pipes[2*j + 1]) < 0)
                    ERR("close");
            }
            if(close(FROM_2_TO_3[1]) < 0) 
                ERR("close");
            if(close(FROM_2_TO_3[0]) < 0) 
                ERR("close");

            // 1st brigade won't use R23 at all

            if(close(FROM_1_TO_2[0]) < 0) 
                ERR("close");
            // 1st brigade won't read from r12 but will write so keep [1] open
            sleep(1); // simulate work

            if(close(FROM_1_TO_2[1]) < 0) 
                ERR("close");
            if(close(w1_pipes[2*i+1]) < 0)
                ERR("close");
            free(w1_pipes);
            free(w2_pipes);
            free(w3_pipes);
            exit(0);
        }
    }

    for(int i = 0; i < w2; i++)
    {
        pid = fork();

        if(pid < 0)
            ERR("fork");
        if(pid == 0)
        {
            //child
            // step 1 close unused pipes;

            

            // workers only write to the boss so close the reading ends
            for(int j = 0; j < w2; j++)
            {
                if(close(w2_pipes[2*j]) < 0)
                    ERR("close");
                if(i != j){
                    if(close(w2_pipes[2*j + 1]) < 0)
                        ERR("close");
                    // close writing end except your own
                }
            }
            for(int j = 0; j < w1; j++)
            {
                if(close(w1_pipes[2*j]) < 0)
                    ERR("close");
                
                if(close(w1_pipes[2*j + 1]) < 0)
                    ERR("close");
                
            }
            for(int j = 0; j < w3; j++)
            {
                if(close(w3_pipes[2*j]) < 0)
                    ERR("close");
                
                if(close(w3_pipes[2*j + 1]) < 0)
                    ERR("close");
            }
            if(close(FROM_2_TO_3[0]) < 0) 
                ERR("close");


            if(close(FROM_1_TO_2[1]) < 0) 
                ERR("close");

            sleep(1); // simulate work

            if(close(FROM_1_TO_2[0]) < 0) 
                ERR("close");
            if(close(FROM_2_TO_3[1]) < 0) 
                ERR("close");
            if(close(w2_pipes[2*i+1]) < 0)
                ERR("close");

            free(w1_pipes);
            free(w2_pipes);
            free(w3_pipes);
            exit(0);
        }
    }

    for(int i = 0; i < w3; i++)
    {
        pid = fork();

        if(pid < 0)
            ERR("fork");
        if(pid == 0)
        {
            //child
            // step 1 close unused pipes;

            
            // workers only write to the boss so close the reading ends
            for(int j = 0; j < w3; j++)
            {
                if(close(w3_pipes[2*j]) < 0)
                    ERR("close");
                if(i != j){
                    if(close(w3_pipes[2*j + 1]) < 0)
                        ERR("close");
                    // close writing end except your own
                }
            }
            for(int j = 0; j < w1; j++)
            {
                if(close(w1_pipes[2*j]) < 0)
                    ERR("close");
                
                if(close(w1_pipes[2*j + 1]) < 0)
                    ERR("close");
                
            }
            for(int j = 0; j < w2; j++)
            {
                if(close(w2_pipes[2*j]) < 0)
                    ERR("close");
                
                if(close(w2_pipes[2*j + 1]) < 0)
                    ERR("close");
            }
            if(close(FROM_2_TO_3[1]) < 0) 
                ERR("close");


            if(close(FROM_1_TO_2[1]) < 0) 
                ERR("close");
            if(close(FROM_1_TO_2[0]) < 0) 
                ERR("close");


            sleep(1); // simulate work

            if(close(FROM_2_TO_3[0]) < 0) 
                ERR("close");
            if(close(w3_pipes[2*i+1]) < 0)
                ERR("close");

            free(w1_pipes);
            free(w2_pipes);
            free(w3_pipes);
            exit(0);
        }
    }

    for(int i = 0; i < 2*w1; i++)
    {
        if(close(w1_pipes[i]) < 0)
            ERR("close");
    }
    for(int i = 0; i < 2*w2; i++)
    {
        if(close(w2_pipes[i]) < 0)
            ERR("close");
    }
    for(int i = 0; i < 2*w3; i++)
    {
        if(close(w3_pipes[i]) < 0)
            ERR("close");
    }
    if(close(FROM_1_TO_2[0]) < 0)
        ERR("close");
    if(close(FROM_1_TO_2[1]) < 0)
        ERR("close");
    if(close(FROM_2_TO_3[0]) < 0)
        ERR("close");
    if(close(FROM_2_TO_3[1]) < 0)
        ERR("close");
    free(w1_pipes);
    free(w2_pipes);
    free(w3_pipes);

    // ALWAYS REMEMBER TO CLOSE / DEALLOCATE EVERYTHING FROM BOTH CHILDREN AND THE PARENT !!!!

}

int main(int argc, char* argv[])
{
    if(argc != 4){
        usage(argv[0]);
    }
    int w1, w2, w3;
    w1 = atoi(argv[1]);
    w2 = atoi(argv[2]);
    w3 = atoi(argv[3]);

    if(w1 < 1 || w1 > 10 || w2 < 1 || w2 > 10 || w3 < 1 || w3 > 10)
        usage(argv[0]);

    
    create_workers(w1,w2,w3);
    while(wait(NULL ) > 0);
    printf("Boss: descriptors: %d\n", count_descriptors());
}

// https://github.com/michau-s/OPS2-L1-Consultations/blob/master/sop2-5/src/sop-roncevaux.c
