#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILENAME "./dossier.txt"
#define MSG_LEN 128

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))



void usage(char* name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "10000 >= n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int fd;
    if((fd = open(FILENAME, O_RDONLY, -1)) < 0){
        ERR("open");
    }
    char *ptr;
    if(MAP_FAILED == (ptr = (char*)mmap(NULL, MSG_LEN, PROT_READ, MAP_SHARED, fd,0))){
        ERR("mmap");
    }
    printf("%s\n", ptr);
    close(fd);
    if(munmap(ptr, MSG_LEN) < 0){
        ERR("munmap");
    }
    return EXIT_SUCCESS;
}