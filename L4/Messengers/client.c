#include "common.h"
#define BACKLOG 5
#define MESS_SIZE 4
#define MAX_CLIENTS 4
#define MAX_EVENTS 16
#define CITIES_COUNT 20

void usage(char* argv){
    printf("Address and a port\n");
    exit(0);
}

int main(int argc, char **argv)
{
    usage(argv[0]);
    return 0;
}

// https://github.com/polatoja/OPS/tree/main/sop4
