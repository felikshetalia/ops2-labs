#include "common.h"
#define BACKLOG 3
#define MAX_CLIENTS 7
#define MAX_EVENTS (MAX_CLIENTS) + 1
#define MAX_MSG 100

void usage(char *name) { fprintf(stderr, "USAGE: %s socket \n", name); }
volatile int doWork = 1;
int main(int argc, char** argv){
    if(argc != 2){
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    uint16_t port = atoi(argv[1]);
    int serverSocket = bind_tcp_socket(port, BACKLOG);
    int new_flags = fcntl(serverSocket, F_GETFL) | O_NONBLOCK;
    fcntl(serverSocket, F_SETFL, new_flags);
    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = serverSocket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, serverSocket, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    int nfds;
    char buf[100];
    int size;
    int total_bytes=0;
    int activeConnections=0;
    while(doWork){
        printf("flag inside 1st loop\n");
        if((nfds = epoll_wait(epoll_descriptor, events, MAX_EVENTS, -1)) < 0){
            ERR("epoll_wait");
        }
        for(int i = 0; i<nfds;i++){
            printf("flag inside for\n");
            if(events[i].data.fd == serverSocket){
                printf("flag inside 1st if\n");
                int clientSocket = add_new_client(events[i].data.fd);
                if(clientSocket){
                    struct epoll_event ce = {.events = EPOLLIN, .data.fd = clientSocket};
                    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, clientSocket, &ce) == -1) {
                        close(clientSocket);
                        perror("epoll_ctl: client");
                        continue;
                    }
                    activeConnections++;
                    if(activeConnections > 1){
                        char *msg = "server is full gtfo\n";
                        epoll_ctl(epoll_descriptor, EPOLL_CTL_DEL, clientSocket, &ce);
                        activeConnections--;
                        if(bulk_write(clientSocket, msg, strlen(msg))<0)
                            ERR("write");
                        if (TEMP_FAILURE_RETRY(close(clientSocket)) < 0)
                            ERR("close");
                    }
                }
            }
            else{

                if((size = read(events[i].data.fd, buf, 100)) > 0){
                    total_bytes += size;
                    printf("flag inside read, total bytes=%d\n", total_bytes);
                    if(total_bytes > 5){
                        printf("nooooo the ritual\n");
                        activeConnections--;
                        if (TEMP_FAILURE_RETRY(close(events[i].data.fd)) < 0)
                            ERR("close");
                        doWork = 0;
                    }
                    if(total_bytes == 5){

                        printf("[%d] Server read %d bytes from the client [%d]\n", serverSocket, total_bytes, events[i].data.fd);
                        activeConnections--;
                        if (TEMP_FAILURE_RETRY(close(events[i].data.fd)) < 0)
                            ERR("close");
                        doWork = 0;
                    }
                }
                if(size == 0){
                    activeConnections--;
                    if (TEMP_FAILURE_RETRY(close(events[i].data.fd)) < 0)
                        ERR("close");
                    doWork = 0;
                }
                if(size<0){
                    ERR("bulk_read");
                }

            }
        }
    }

    if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
    if (TEMP_FAILURE_RETRY(close(serverSocket)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}