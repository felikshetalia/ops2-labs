#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>

void usage(char** argv){
    fprintf(stderr, "Usage: %s <message>\n", argv[0]);
    return;
}

int main(int argc, char** argv){
    if (argc != 2){
        usage(argv);
    }
    int fd[2];
    if(pipe(fd) < 0){
        perror("pipe(): ");
        return 1;
    }
    // fd[0] represents read-end of the pipe
    // fd[1] represents write-end of the pipe

    const char* msg = argv[1];
    int msg_len = strlen(msg) + 1; //+1 is for \0 null terminator

    pid_t pid = fork();
    // this program will make parent send message to child
    // i.e child - reader; parent - writer
    switch (pid)
    {
    case -1:
        perror("fork(): ");
        return 1;
    case 0:
        // code is running inside the child process
        close(fd[1]); //close unused write-end
        char buffer[50];
        ssize_t ret;
        if((ret = read(fd[0], buffer, sizeof(buffer)))<0){
            perror("read(): ");
            return 1;
        }
        printf("[%d] read() returned %ld bytes: '%s'\n", getpid(), ret, buffer);
        close(fd[0]);
        break;
    default:
        // code is running inside the parent (main) process
        close(fd[0]);
        //ssize_t ret;
        if((write(fd[1], msg, msg_len))<0){
            perror("write(): ");
            return 1;
        }
        printf("[%d] write() returned %ld bytes\n", getpid(), ret);
        close(fd[1]);
        while (wait(NULL) > 0); // wait for all processes to end
        break;
    }

    return 0;

}