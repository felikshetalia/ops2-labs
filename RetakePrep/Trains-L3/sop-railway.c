#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <semaphore.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))



typedef struct{
    int from;
    int dest;
    int weight;
}Network_t;
void railway_network_destroy(Network_t* networksList, int n)
{
    if(munmap(networksList, sizeof(Network_t)*n) < 0)
        ERR("munmap");
    
}
void railway_network_init(int fp, Network_t* networksList)
{
    int connections = 0;
    char c;
    FILE *fd = fdopen(fp, "r");
    while((c = getc(fd)) != EOF){
        if(c == '\n')
            connections++;
    }
    
    // Network_t* networksList;
    if(MAP_FAILED == (networksList = mmap(NULL, connections * sizeof(Network_t), PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0)))
        ERR("mmap");

    printf("Map succesful\n");
    for(int i = 0; i < connections; i++)
    {
        fscanf(fd, "%d %d %d", &networksList[i].from, &networksList[i].dest, &networksList[i].weight);
    }
    fclose(fd);
    railway_network_destroy(networksList, connections);
}
int main(int argc, char** argv)
{
    //usage(argv[0]);
    int nwpt;
    if((nwpt = open("example.rail", O_RDWR)) == -1)
        ERR("open");
    Network_t* networksList;
    railway_network_init(nwpt, networksList);
    close(nwpt);
    return 0;
}
