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
    int child2parent[2], parent2child[2];
    // file descriptors for which the child and parent will write respectively
    // fd[0] read end; fd[1] write end
    if((pipe(child2parent) < 0) || (pipe(parent2child) < 0)){
        perror("pipe(): ");
        return 1;
    }
    pid_t pid = fork();
    switch (pid)
    {
        case -1:
            perror("fork(): ");
            return 1;
        case 0:
        // child writes
            close(parent2child[1]); //close unused write end
            close(child2parent[0]); //close unused read end

            const char msg2parent[] = "Mommy";
            char recvbuf1[100];
            ssize_t ret1;
            if((write(child2parent[1], msg2parent, sizeof(msg2parent) + 1)) < 0){
                perror("write(): ");
                return 1;
            }
            else{
                printf("[%d] Child says: %s\n", getpid(), msg2parent);
            }

            if((ret1 = read(parent2child[0], recvbuf1, sizeof(recvbuf1))) < 0){
                perror("read(): ");
                return 1;
            }
            else{
                printf("[%d] Child received %ld bytes: '%s'\n", getpid(), ret1, recvbuf1);
            }

            close(parent2child[0]);
            close(child2parent[1]);
            break;
        
        default:
            // parent writes
            close(parent2child[0]); //close unused write end
            close(child2parent[1]); //close unused read end

            const char msg2child[] = "Yes darling";
            char recvbuf[100];
            ssize_t ret2;
            if((ret2 = read(child2parent[0], recvbuf, sizeof(recvbuf))) < 0){
                perror("read(): ");
                return 1;
            }
            else{
                printf("[%d] Parent received %ld bytes: '%s'\n", getppid(), ret2, recvbuf);
            }
            if((write(parent2child[1], msg2child, sizeof(msg2child) + 1)) < 0){
                perror("write(): ");
                return 1;
            }
            else{
                printf("[%d] Parent says: %s\n", getppid(), msg2child);
            }

            close(parent2child[1]);
            close(child2parent[0]);
            while(wait(NULL) > 0);
            break;
    }
    return 0;
}