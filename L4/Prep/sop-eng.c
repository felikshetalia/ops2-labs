#include "common.h"

#define MAX_CLIENTS 4
#define MAX_USERNAME_LENGTH 32
#define MAX_MESSAGE_SIZE 64
#define BACKLOG 16
#define MAX_EVENTS 12

void usage(char* program_name)
{
    fprintf(stderr, "Usage: \n");

    fprintf(stderr, "\t%s", program_name);
    set_color(2, SOP_PINK);
    fprintf(stderr, " port\n");

    fprintf(stderr, "\t  port");
    reset_color(2);
    fprintf(stderr, " - the port on which the server will run\n");

    exit(EXIT_FAILURE);
}

int main(int argc, char** argv){
    if(argc != 2){
        usage(argv[0]);
    }

    int socket_fd = bind_tcp_socket(atoi(argv[1]), BACKLOG);

    
    // set up epoll
    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, socket_fd, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    
    char buffer[3];
    int nfds = epoll_wait(epoll_descriptor, events, MAX_EVENTS, -1);
    int allowed = 0;
    for(int i = 0; i<nfds;i++){
        // remember to allow 1 client
        int accepted_client_fd = add_new_client(events[i].data.fd);
        if (accepted_client_fd > 0) allowed++;
        if(allowed > 1){
            close(accepted_client_fd);
        }
        else{
            while(1){
                ssize_t ret;
                if((ret = read(accepted_client_fd, buffer, 3)) < 0){
                    ERR("read");
                }
                if(ret == 0) break;

                printf("%s\n", buffer);

                if(strcmp(buffer, "\n") == 0) break;
            }

            if(TEMP_FAILURE_RETRY(close(accepted_client_fd)) < 0){
                ERR("close");
            }
        }
    }
    if(TEMP_FAILURE_RETRY(close(socket_fd)) < 0){
        ERR("close");
    }
    return EXIT_SUCCESS;
}
