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
#define PIPE_CAPACITY 65536

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv){
    int fd[2];
    // 0 - read
    // 1 - write
    if(pipe(fd) < 0){
        ERR("pipe");
    }
    //const char *msg = argv[1];
    char buffer[PIPE_BUF + 1];
    memset(buffer, 0, sizeof(buffer));
    size_t total = 0;
    long int lim = PIPE_CAPACITY / (PIPE_BUF / 2) - 1;
    printf("Limit = %ld\n", lim);
    printf("Written: %s\n", buffer);
    for(int i = 0; i < PIPE_CAPACITY / (PIPE_BUF / 2) - 1; i++){
        ssize_t ret = write(fd[1], buffer, PIPE_BUF / 2);
        if(ret < 0) ERR("write");
        total += ret;
        printf("Total bytes written: %ld\n", total);
    }
    printf("Trying to write more bytes\n");
    ssize_t ret = write(fd[1], buffer, PIPE_BUF); // TODO: Check what happens when trying to write 1 byte more
    if (ret < 0) {
        perror("write(): ");
        return 1;
    }
    printf("Done! Written %ld bytes\n", ret);

    return 0;

}