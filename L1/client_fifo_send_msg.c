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
// program that will SEND
int main(int argc, char** argv){
    // if(unlink(MAIN_FIFO_NAME) < 0){
    //     ERR("unlink");
    // }
    int fd;
    //printf("Trying to open FIFO '%s' for reading\n", MAIN_FIFO_NAME);
    if((fd = open(MAIN_FIFO_NAME, O_RDWR)) < 0){
        ERR("open");
    }
    char msg[50];
    char buf[50];
    ssize_t ret;
    while(1){
        printf("Type your message: ");
        scanf("%s", msg);
        //printf("Writing to fifo...\n");
        if(write(fd, msg, (strlen(msg) + 1)) < 0){
            //cleanup(MAIN_FIFO_NAME, fd);
            close(fd);
            ERR("write");
        }
        else{
            printf("Client: %s\n", msg);
        }
        if((ret = read(fd, buf, sizeof(buf))) < 0){
            //cleanup(MAIN_FIFO_NAME, fd);
            ERR("read");
        }
        else{
            printf("Server: %s\n", buf);
        }
    }
    
    close(fd);
    //cleanup(MAIN_FIFO_NAME, fd);
    return 0;
}