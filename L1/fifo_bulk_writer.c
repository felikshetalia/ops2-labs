#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))
#define FIFO_NAME "./myfifo"

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

void write_to_fifo(int fd, int txtfile){
    int64_t ret;
    char buffer[PIPE_BUF];
    char *bit;
    *((pid_t *)buffer) = getpid();
    bit = buffer + sizeof(pid_t);

    do{
        if((ret = read(txtfile,bit,MSG_SIZE)) < 0){
            ERR("read");
        }
        if(ret < MSG_SIZE){
            memset(bit+ret, 0, MSG_SIZE-ret);
        }
        if(ret > 0){
            if(write(fd,buffer,PIPE_BUF) < 0){
                ERR("write");
            }
        }
    }while(ret == MSG_SIZE);
}

int main(int argc, char** argv){
    int fd, txt;
    if(argc != 2){
        usage(argv[0]);
    }
    // open fifo's write end, the fifo was created by the server
    if((fd = open(FIFO_NAME, O_WRONLY)) < 0){
        ERR("open");
    }
    // open a random file to read from
    if((txt = open(argv[1], O_RDONLY)) < 0){
        ERR("open");
    }
    write_to_fifo(fd,txt);
    if (close(txt) < 0)
        perror("Close fifo:");
    if (close(fd) < 0)
        perror("Close fifo:");

    return 0;
}