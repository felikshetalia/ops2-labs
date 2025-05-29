#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FIFO_NAME "./myfifo"

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

void read_whole(int fd){
    ssize_t ret;
    char buf[50];
    if((ret = read(fd, buf, sizeof(buf))) < 0){
        ERR("read");
    }
    else{
        printf("%s", buf);
    }
}

void read_from_fifo(int fifo){
    /*1. this is a fifo read function that reads 
    byte by byte instead of the whole thing */

    ssize_t ret;
    char c;
    do{
        if((ret = read(fifo, &c, 1)) < 0){
            ERR("read");
        }
        if(ret > 0 && isalnum(c)){
            printf("%c", c);
        }
    }while(ret > 0);
}

int main(int argc, char** argv){
    unlink(FIFO_NAME);
    int fd;
    if(mkfifo(FIFO_NAME, 0660) < 0){
        if(errno != EEXIST){
            ERR("mkfifo");
        }
    }
    if((fd = open(FIFO_NAME, O_RDONLY)) < 0){
        ERR("open");
    }
    //read_from_fifo(fd);
    printf("\n");
    read_whole(fd);
    printf("\n");
    if (close(fd) < 0)
        ERR("close fifo:");
    return 0;
}
