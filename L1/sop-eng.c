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
    while (1)
    {
        char c = 'A' + rand() % ('Z' - 'A');
        msleep(rand() % 10 + 1);
        int ret;
        if ((ret = write(production_pipe_write, &c, 1)) < 0)
        {
            ERR("write");
        }
        else
        {
            printf("1st brigade wrote %c\n", c);
            exit(0);
        }
    }
    printf("Worker %d from the first brigade: descriptors: %d\n", getpid(), count_descriptors());
}
void second_brigade_work(int production_pipe_write, int production_pipe_read, int boss_pipe)
{
    srand(getpid());
    int ret;
    char response;
    while (1)
    {
        // ret = read(production_pipe_read,&response,1);
        if ((ret = read(production_pipe_read, &response, 1)) < 0)
        {
            ERR("read");
        }
        else if (ret == 1)
        {
            printf("2nd brigade read %c\n", response);
            msleep(rand() % 10 + 1);
            int stat;
            int rand_iter = rand() % 4 + 1;
            char* ig = calloc(rand_iter, sizeof(char));
            if ( ig == NULL ){
                ERR("calloc");
            }
            if ((stat = write(production_pipe_write, &response, 1)) < 0)
                {
                    ERR("write");
                }
                if (ret == 0)
                {
                    free(ig);
                    printf("write returned 0");
                    break;
                }
            // int total = 0;
            // for (int i = 0; i < rand_iter; i++)
            // {
            //     if ((stat = write(production_pipe_write, &response, 1)) < 0)
            //     {
            //         ERR("write");
            //     }
            //     if (ret == 0)
            //     {
            //         free(ig);
            //         break;
            //     }
            // }
            
        }
        else{
            // printf("Read returned 0\n");
            // exit(0);
            break;
        }
        // else{
        //     msleep(rand() % 10 + 1);
        //     int ret;
        //     int rand_iter = rand() % 4 + 1;
        //     int total = 0;
        //     for(int i=0;i<rand_iter;i++){

        //     }
        // }
    }

    printf("Worker %d from the second brigade: descriptors: %d\n", getpid(), count_descriptors());
}
void third_brigade_work(int production_pipe_read, int boss_pipe)
{
    srand(getpid());
    int ret;
    char* response;
    while (1)
    {
        while (1)
        {
            msleep(rand() % 3 + 1);
            ret = read(production_pipe_read, response, strlen(response)+1);
            if (ret == 1)
            {
                if (strlen(response)== 5)
                {
                    printf("%s\n", response);
                    exit(0);
                }
                else
                {
                    continue;
                }
            }
            
        }

        
    }
    printf("Worker %d from the third brigade: descriptors: %d\n", getpid(), count_descriptors());
}

void create_children(int w1, int w2, int w3)
{
    int *fds1, *fds2, *fds3;
    int R12[2], R23[2];
    if ((fds1 = calloc(w1 * 2, sizeof(int))) == NULL)
    {
        ERR("calloc");
    }
    if ((fds2 = calloc(w2 * 2, sizeof(int))) == NULL)
    {
        ERR("calloc");
    }
    if ((fds3 = calloc(w3 * 2, sizeof(int))) == NULL)
    {
        ERR("calloc");
    }
    // create pipes
    for (int i = 0; i < w1; i++)
    {
        if (pipe(&fds1[2 * i]) < 0)
        {
            ERR("pipe");
        }
    }
    for (int i = 0; i < w2; i++)
    {
        if (pipe(&fds2[2 * i]) < 0)
        {
            ERR("pipe");
        }
    }
    for (int i = 0; i < w3; i++)
    {
        if (pipe(&fds3[2 * i]) < 0)
        {
            ERR("pipe");
        }
    }
    // main pipes
    if (pipe(R12) < 0)
    {
        ERR("pipe");
    }
    if (pipe(R23) < 0)
    {
        ERR("pipe");
    }

    // closing unused descs
    for (int i = 0; i < w1; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            ERR("fork");
        }
        if (pid == 0)
        {
            // child
            for (int j = 0; j < w1; j++)
            {
                if (i != j)
                {
                    if (close(fds1[2 * j + 1]) < 0)
                    {
                        ERR("close");
                    }
                }
                if (close(fds1[2 * j]) < 0)
                {
                    ERR("close");
                }
            }
            for (int j = 0; j < w2; j++)
            {
                if (close(fds2[2 * j]) < 0)
                {
                    ERR("close");
                }
                if (close(fds2[2 * j + 1]) < 0)
                {
                    ERR("close");
                }
            }
            for (int j = 0; j < w3; j++)
            {
                if (close(fds3[2 * j]) < 0)
                {
                    ERR("close");
                }
                if (close(fds3[2 * j + 1]) < 0)
                {
                    ERR("close");
                }
            }
            if (close(R12[0]) < 0)
            {
                ERR("close");
            }
            if (close(R23[0]) < 0)
            {
                ERR("close");
            }
            if (close(R23[1]) < 0)
            {
                ERR("close");
            }
            // w1 wont use 23
            first_brigade_work(R12[1],fds1[2 * i + 1]);
            free(fds1);
            free(fds2);
            free(fds3);
            exit(0);
        }
    }

    // 2nd brigade
    for (int i = 0; i < w2; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            ERR("fork");
        }
        if (pid == 0)
        {
            // child
            for (int j = 0; j < w1; j++)
            {
                if (close(fds1[2 * j]) < 0)
                {
                    ERR("close");
                }
                if (close(fds1[2 * j + 1]) < 0)
                {
                    ERR("close");
                }
            }
            for (int j = 0; j < w2; j++)
            {
                if (i != j)
                {
                    if (close(fds2[2 * j + 1]) < 0)
                    {
                        ERR("close");
                    }
                }
                if (close(fds2[2 * j]) < 0)
                {
                    ERR("close");
                }
            }
            for (int j = 0; j < w3; j++)
            {
                if (close(fds3[2 * j]) < 0)
                {
                    ERR("close");
                }
                if (close(fds3[2 * j + 1]) < 0)
                {
                    ERR("close");
                }
            }
            if (close(R23[0]) < 0)
            {
                ERR("close");
            }
            if (close(R12[1]) < 0)
            {
                ERR("close");
            }

            second_brigade_work(R23[1], R12[0], fds2[2 * i + 1]);
            free(fds1);
            free(fds2);
            free(fds3);
            exit(0);
        }
    }
    for (int i = 0; i < w3; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            ERR("fork");
        }
        if (pid == 0)
        {
            // child
            for (int j = 0; j < w1; j++)
            {
                if (close(fds1[2 * j]) < 0)
                {
                    ERR("close");
                }
                if (close(fds1[2 * j + 1]) < 0)
                {
                    ERR("close");
                }
            }
            for (int j = 0; j < w2; j++)
            {
                if (close(fds2[2 * j]) < 0)
                {
                    ERR("close");
                }
                if (close(fds2[2 * j + 1]) < 0)
                {
                    ERR("close");
                }
            }
            for (int j = 0; j < w3; j++)
            {
                if (i != j)
                {
                    if (close(fds3[2 * j + 1]) < 0)
                    {
                        ERR("close");
                    }
                }
                if (close(fds3[2 * j]) < 0)
                {
                    ERR("close");
                }
            }
            if (close(R23[1]) < 0)
            {
                ERR("close");
            }
            if (close(R12[1]) < 0)
            {
                ERR("close");
            }
            if (close(R12[0]) < 0)
            {
                ERR("close");
            }
            // w1 wont use 23
            third_brigade_work(R23[0], fds3[2 * i + 1]);
            free(fds1);
            free(fds2);
            free(fds3);
            exit(0);
        }
    }
    for (int j = 0; j < w1; j++)
    {
        if (close(fds1[2 * j + 1]) < 0)
        {
            ERR("close");
        }
    }
    for (int j = 0; j < w2; j++)
    {
        if (close(fds2[2 * j + 1]) < 0)
        {
            ERR("close");
        }
    }
    for (int i = 0; i < w3; i++)
    {
        if (close(fds3[2 * i + 1]) < 0)
        {
            ERR("close");
        }
    }
    if (close(R12[0]) < 0)
    {
        ERR("close");
    }
    if (close(R12[1]) < 0)
    {
        ERR("close");
    }
    if (close(R23[0]) < 0)
    {
        ERR("close");
    }
    if (close(R23[1]) < 0)
    {
        ERR("close");
    }
    free(fds1);
    free(fds2);
    free(fds3);
}

int main(int argc, char* argv[])
{
    if (argc != 4)
        usage(argv[0]);
    int w1 = atoi(argv[1]), w2 = atoi(argv[2]), w3 = atoi(argv[3]);
    if (w1 < 1 || w1 > 10 || w2 < 1 || w2 > 10 || w3 < 1 || w3 > 10)
        usage(argv[0]);

    create_children(w1, w2, w3);
    while (wait(NULL) > 0)
        ;
    printf("Boss: descriptors: %d\n", count_descriptors());
}

// https://github.com/michau-s/OPS2-L1-Consultations/blob/master/sop2-5/src/sop-roncevaux.c