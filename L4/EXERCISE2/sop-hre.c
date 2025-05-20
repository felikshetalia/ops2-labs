#include "common.h"
#define BACKLOG 3
#define MAX_CLIENTS 7
#define MAX_EVENTS (MAX_CLIENTS) + 1
#define MAX_MSG 100
void usage(char *name) { fprintf(stderr, "USAGE: %s port\n", name); }

typedef struct {
    int fd;
    char buf[MAX_MSG];
} Client_t;

void sigpipe_handler(int sig)
{
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, sig);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
}

void send_welcome(int fd){
    char* msg = "Welcome, elector\n";
    if(write(fd, msg, strlen(msg)+1) < 0){
        if(errno != EPIPE)
            ERR("write");
        if (TEMP_FAILURE_RETRY(close(fd)) < 0)
            ERR("close");
    }
}

void recv_from_client(int fd){
    char buffer[MAX_MSG+1];
    int ret;
    if((ret = read(fd, buffer, MAX_MSG)) < 0){
        ERR("read");
    }
    if(ret == 0){
        printf("[%d] Client disconnected\n", fd);
        if (TEMP_FAILURE_RETRY(close(fd)) < 0)
            ERR("close");
    }
    buffer[ret] = '\n';
    printf("[%d] %s\n",fd, buffer);
}

int main(int argc, char **argv)
{
    sigpipe_handler(SIGPIPE);
    // only port
   if(argc != 2)
        usage(argv[0]);

    int server_fd = bind_tcp_socket(atoi(argv[1]), BACKLOG);
    int new_flags = fcntl(server_fd, F_GETFL) | O_NONBLOCK;
    fcntl(server_fd, F_SETFL, new_flags);
    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, server_fd, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    // work
    Client_t ClientList[MAX_CLIENTS];
    int active_connections = 0;
    while(1){
        int nfds = epoll_wait(epoll_descriptor, events, MAX_EVENTS, -1);
        if(nfds > 0){
            for(int i = 0; i < nfds; i++){
                if(events[i].data.fd == server_fd){
                    // new client joined
                    int accepted_client = add_new_client(events[i].data.fd);
                    if(accepted_client){
                        if(active_connections >= MAX_CLIENTS){
                            // we don't add any
                            if (TEMP_FAILURE_RETRY(close(accepted_client)) < 0)
                                ERR("close");
                        }
                        else{

                            printf("[%d] Client connected\n", accepted_client);
                            Client_t newClient;
                            newClient.fd = accepted_client;
                            ClientList[active_connections] = newClient;
                            active_connections++;

                            send_welcome(accepted_client);
                            
                        }
                        
                        struct epoll_event client_ev;
                        client_ev.events = EPOLLIN;
                        client_ev.data.fd = accepted_client;
                        if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, accepted_client, &client_ev) == -1)
                        {
                            perror("epoll_ctl: listen_sock");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else{
                    // get msgs from already connected user
                    
                    recv_from_client(events[i].data.fd);

                }
        
            }
    
        }

    }


    if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
    if (TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}