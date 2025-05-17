#include "common.h"
#define BACKLOG 3
void usage(char *name) { fprintf(stderr, "USAGE: %s port\n", name); }

int main(int argc, char **argv)
{
    // only port
    if(argc != 2) usage(argv[0]);

    int sockfd = bind_tcp_socket(atoi(argv[1]), BACKLOG);

    int accepted;
    if((accepted = add_new_client(sockfd)) >= 0){
        printf("Client connected\n");
    }
    close(accepted);
    close(sockfd);
    return EXIT_SUCCESS;
}