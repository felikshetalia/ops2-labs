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
    int n; //number of children defined by user
    if(argc != 2){
        usage(argv);
    }
    n = atoi(argv[1]);
    int fd[n][2]; // file descriptors for each child
    pid_t pid;
    // create pipes for each
    for(int i = 0; i < n; i++){
        if(pipe(fd[i]) < 0){
            perror("pipe(): ");
            return 1;
        }
    }
    for(int i = 0; i < n; i++){
        pid = fork();
        switch (pid)
        {
        case -1:
            perror("fork(): ");
            return 1;
        case 0:
            //child will read the msg
            char buf[20];
            close(fd[i][1]); //close unused write end
            size_t ret;
            if((ret = read(fd[i][0], buf, sizeof(buf)))<0){
                perror("read(): ");
                return 1;
            }
            printf("[%d] is given the name '%s'\n", getpid(), buf);
            exit(0);
        default:
            // parent process writes stuff
            close(fd[i][0]); // close unused read-end
            printf("Enter name: ");
            char name[20];
            scanf("%s", name);
            if(write(fd[i][1], name, strlen(name)+1)<0){
                perror("write(): ");
                return 1;
            }
            while (wait(NULL) > 0);
            break;
        }
    }
}