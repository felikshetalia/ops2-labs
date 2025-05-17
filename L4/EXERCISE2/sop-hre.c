#include "common.h"
#define BACKLOG 3
void usage(char *name) { fprintf(stderr, "USAGE: %s port\n", name); }

int main(int argc, char **argv)
{
    // only port
    if(argc != 2) usage(argv[0]);

    int sockfd = bind_tcp_socket(atoi(argv[1]), BACKLOG);

    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[16];
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, sockfd, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    int nfds;
    //int active_clients;
    while(1){
        if((nfds = epoll_pwait(epoll_descriptor,events, 16, -1, NULL)) > 0){
            for (int i = 0; i < nfds; i++){

                int accepted;
                if((accepted = add_new_client(events[i].data.fd)) >= 0){
                    //printf("Client connected\n");
                    int vote = rand() % 8;
                    char msg[30];
                    fnprintf(msg, 30, "Welcome, elector of %d", vote);
                    if(bulk_write(accepted, msg, strlen(msg)+1) < 0){
                        ERR("write");
                    }
                    else{
                        // fprintf(stdout, msg);
                        printf("%s\n", msg);
                    }
                }
                close(accepted);
            }

        }
        else {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }
    }
    close(sockfd);
    if(close(epoll_descriptor) < 0) ERR("close");
    return EXIT_SUCCESS;
}