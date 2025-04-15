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

void count_occurences(char* ptr, char* shared, int idx){
    int freq[256] = {0}; // One slot for each ASCII character

    for (int i = 0; ptr[i] != '\0'; i++) {
        unsigned char ch = ptr[i];
        freq[ch]++;
    }

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            snprintf(shared + (idx * 256 + i) * MSG_LEN, MSG_LEN,
                     "'%c' → %d times\n", i, freq[i]);
        } else {
            shared[(idx * 256 + i) * MSG_LEN] = '\0'; // clean unused slot
        }
    }
}

void child_work(int idx, char* shared){
    int fd;
    if((fd = open(FILENAME, O_RDONLY, -1)) < 0){
        ERR("open");
    }
    char *ptr;
    if(MAP_FAILED == (ptr = (char*)mmap(NULL, MSG_LEN, PROT_READ, MAP_SHARED, fd,0))){
        ERR("mmap");
    }
    //printf("%s\n", ptr);
    count_occurences(ptr, shared, idx);
    close(fd);
    if(munmap(ptr, MSG_LEN) < 0){
        ERR("munmap");
    }
}

void parent_work(int n,char* shared){
    for (int i = 0; i < n; i++) {
        printf("Child %d results:\n", i);
        for (int j = 0; j < 256; j++) {
            char *msg = shared + (i * 256 + j) * MSG_LEN;
            if (msg[0] != '\0') {
                printf("%s", msg);
            }
        }
    }
    
}

void pop_children(int n, char* shared){
    srand(getpid());
    for (int i = 0; i < n; i++){
        pid_t pid = fork();
        if(pid == 0){
            // child
            child_work(i, shared);
            exit(0);
        }
        if(pid < 0) ERR("fork");
    }
}

int main(int argc, char** argv)
{
    if (argc != 2) usage(argv[0]);
    int n = atoi(argv[1]);
    char* shared;
    // for all the results
    if(MAP_FAILED == (shared = (char*)mmap(NULL, n*256*MSG_LEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0))){
        ERR("mmap");
    }
    pop_children(n, shared);
    while(wait(NULL) > 0);
    parent_work(n, shared);
    if(munmap(shared, n*256*MSG_LEN) < 0){
        ERR("munmap");
    }
    return EXIT_SUCCESS;
}