#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>

#define SERVER_FIFO "/tmp/server_fifio"
#define CLIENT_FIFO "/tmp/client_fifio"
#define MAIN_FIFO_NAME "/tmp/my_fifo"
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void cleanup(const char* path, int fd){
    if(0>close(fd)){
        ERR("close");
    }
    if(0 > unlink(path)){
        ERR("unlink");
    }
}
// program that will RECEIVE
int main(int argc, char** argv){
    // if(unlink(MAIN_FIFO_NAME) < 0){
    //     ERR("unlink");
    // }
    int fd;

    if(mkfifo(MAIN_FIFO_NAME, 0666) < 0){
        if(errno != EEXIST)
            ERR("mkfifo");
    }

    printf("Trying to open FIFO '%s' for reading\n", MAIN_FIFO_NAME);
    if((fd = open(MAIN_FIFO_NAME, O_RDWR)) < 0){
        ERR("open");
    }
    char buf[50];
    ssize_t ret;
    char msg[50];
    while(1){
        if((ret = read(fd, buf, sizeof(buf))) < 0){
            cleanup(MAIN_FIFO_NAME, fd);
            ERR("read");
        }
        else{
            printf("Client: %s\n", buf);
        }
        printf("Type your message: ");
        scanf("%s", msg);
        if(write(fd, msg, (strlen(msg) + 1)) < 0){
            //cleanup(MAIN_FIFO_NAME, fd);
            close(fd);
            ERR("write");
        }
        else{
            printf("Server: %s\n", msg);
        }
        sleep(1);
    }
    
    cleanup(MAIN_FIFO_NAME, fd);
    return 0;
}