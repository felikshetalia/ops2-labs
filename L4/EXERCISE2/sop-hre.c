#include "common.h"
#define BACKLOG 3
#define MAX_CLIENTS 7
#define MAX_EVENTS (MAX_CLIENTS) + 1
#define MAX_MSG 100
void usage(char *name) { fprintf(stderr, "USAGE: %s port\n", name); }

typedef struct {
    int fd;
    char buf[MAX_MSG];
    int district_idx;
} Client_t;

const char* elector_names[] = {
    "Mainz",
    "Trier",
    "Cologne",
    "Bohemia",
    "Palatinate",
    "Saxony",
    "Brandenburg"
};

const char* candidates[] = {
    "Francis I, King of France",
    "Charles V, Archduke of Austria and King of Spain",
    "Henry VIII, King of England"
};

void sigpipe_handler(int sig)
{
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, sig);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
}

void remove_client(Client_t *ClientList, int *active_connections, int fd, int epfd){
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    // if(TEMP_FAILURE_RETRY(close(fd))<0) ERR("close");
    for (int i = 0; i < *active_connections; ++i) {
        if (ClientList[i].fd == fd) {
            for (int j = i; j < *active_connections - 1; ++j) {
                ClientList[j] = ClientList[j + 1];
            }
            (*active_connections)--;
            break;
        }
    }
}

void send_welcome(int fd, int i){
    char msg[MAX_MSG];
    snprintf(msg, MAX_MSG, "Welcome, elector of %s\n", elector_names[i-1]);
    //int r = (rand() % 7) + 1;

    if(write(fd, msg, strlen(msg)+1) < 0){
        if(errno != EPIPE)
            ERR("write");
        if (TEMP_FAILURE_RETRY(close(fd)) < 0)
            ERR("close");
    }
}

void recv_votes(Client_t* ClientList, int active_electors, int fd, int epfd){
    char buf;
    int ret;
    do{
        if((ret = read(fd, &buf, 1)) < 0){
            ERR("read");
        }
        if(ret == 0){
            printf("[%d] Client disconnected\n", fd);
            remove_client(ClientList, &active_electors, fd, epfd);
            if (TEMP_FAILURE_RETRY(close(fd)) < 0)
                ERR("close");
            return;
        }

    } while(buf == '\n' || buf == '\r');
    int vote = atoi(&buf);
    if(vote < 1 || vote > 3){
        printf("[%d] Client disconnected\n", fd);
        if (TEMP_FAILURE_RETRY(close(fd)) < 0)
            ERR("close");
        return;
    }
    char msg[MAX_MSG];
    snprintf(msg, MAX_MSG, "You have voted for %s\n", candidates[vote-1]);
    //int r = (rand() % 7) + 1;

    if(write(fd, msg, strlen(msg)+1) < 0){
        if(errno != EPIPE)
            ERR("write");
        if (TEMP_FAILURE_RETRY(close(fd)) < 0)
            ERR("close");
    }

}

void handle_welcome_msgs(int fd, Client_t *elector, Client_t* ClientList, int epfd, int actives)
{
    // client connects, read a char

    char buf; // will be the digit
    if(read(elector->fd, &buf, 1) < 0){
        ERR("read");
    }
    int index = atoi(&buf);
    if(index < 1 || index > 7){
        printf("[%d] Client disconnected\n", elector->fd);
        remove_client(ClientList, &actives, elector->fd, epfd);
        if (TEMP_FAILURE_RETRY(close(elector->fd)) < 0)
            ERR("close");
    }
    elector->district_idx = index;
    send_welcome(elector->fd, index);
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
                            // Client_t newClient;
                            // newClient.fd = accepted_client;
                            // newClient.district_idx = 0; // hasn't been given yet
                            // ClientList[active_connections] = newClient;
                            ClientList[active_connections].fd = accepted_client;
                            ClientList[active_connections].district_idx = 0; // hasn't been given yet
                            // send_welcome(accepted_client, active_connections);
                            
                            struct epoll_event client_ev;
                            client_ev.events = EPOLLIN;
                            client_ev.data.fd = accepted_client;
                            if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, accepted_client, &client_ev) == -1)
                            {
                                perror("epoll_ctl: listen_sock");
                                exit(EXIT_FAILURE);
                            }
                            handle_welcome_msgs(accepted_client, &ClientList[active_connections], ClientList, epoll_descriptor,active_connections);
                            active_connections++;
                        }
                    }  
                    
                }
                else{
                    // get msgs from already connected user
                    int theCli = -1;
                    for(int j = 0; j < active_connections; j++){
                        if(ClientList[j].fd == events[i].data.fd){
                            theCli = j;
                            break;
                        }
                    }
                    if(theCli == -1) continue;
                    if(ClientList[theCli].district_idx == 0){
                        handle_welcome_msgs(ClientList[theCli].fd, &ClientList[theCli], ClientList, epoll_descriptor, active_connections);
                    }
                    else
                        recv_votes(ClientList, active_connections, ClientList[theCli].fd, epoll_descriptor);

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