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
    if(argc != 3){
        usage(argv[0]);
    }
    int sucket = connect_tcp_socket(argv[1], argv[2]);
    char buffer[MESS_SIZE];
    if(fgets(buffer, MESS_SIZE, stdin) == NULL){
        close(sucket);
        ERR("fgets");
    }
    if(bulk_write(sucket, buffer, 4) < 0){
        ERR("write");
    }

    // close the connection
    if(TEMP_FAILURE_RETRY(close(sucket)) < 0){
        ERR("close");
    }
    return 0;
}

// https://github.com/polatoja/OPS/tree/main/sop4
