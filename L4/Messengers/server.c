#include "common.h"

#define MAX_CLIENTS 4
#define MAX_EVENTS (MAX_CLIENTS) + 1
#define MAX_USERNAME_LENGTH 32
#define MAX_MESSAGE_SIZE 64
#define BACKLOG 3

const char* greek_cities[20] = {
    "Athens",
    "Sparta",
    "Corinth",
    "Thebes",
    "Argos",
    "Megara",
    "Rhodes",
    "Ephesus",
    "Miletus",
    "Syracuse",
    "Chalcis",
    "Aegina",
    "Delphi",
    "Olympia",
    "Byzantium",
    "Pergamon",
    "Knossos",
    "Mycenae",
    "Phocaea",
    "Halicarnassus"
};

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

typedef struct {
    int fd;
} Client_t;

void add_new_client_handler(struct epoll_event ev, Client_t* ClientList){

}

void handle_client_messages(int accepted_client){
    char input[4];
    int ret;
    do{
        ret = bulk_read(accepted_client, input, 4);
        if(ret < 0)
            ERR("read");
        if(ret == 0){
            if(TEMP_FAILURE_RETRY(close(accepted_client)) < 0)
                ERR("close");
    
        }
        if(ret > 0){
            printf("%s\n", input);
        }

    } while(ret <= sizeof(char[4]));
}

int main(int argc, char** argv){
    if(argc != 2){
        usage(argv[0]);
    }
    int port = atoi(argv[1]);
    if(port < 1024){
        usage(argv[0]);
    }

    int server_fd = bind_tcp_socket(port, BACKLOG);
    int new_flags = fcntl(server_fd, F_GETFL) | O_NONBLOCK;
    fcntl(server_fd, F_SETFL, new_flags);
    // set up epoll
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

    // 1st stage is a single client. no epoll needed
    int active_connections = 0;
    int nfds;
    Client_t ClientList[MAX_CLIENTS];
    while(1){
        if((nfds = epoll_wait(epoll_descriptor, events, MAX_EVENTS, -1)) > 0){
            for(int i = 0; i < nfds; i++){
                if(events[i].data.fd == server_fd){
                    if(active_connections >= MAX_CLIENTS){
                        // we reject
                        if (write(events[i].data.fd, "Server is full\n", 16) < 0)
                        {
                            if (errno != EPIPE)
                            {
                                ERR("write");
                            }
                            if (TEMP_FAILURE_RETRY(close(events[i].data.fd)) < 0)
                                ERR("close");
                        }
                    }
                    else{
                        // add a NEW client
                        int accepted_client = add_new_client(events[i].data.fd);
                        ClientList[active_connections].fd = accepted_client;
                        printf("[%d] New client connected\n",ClientList[active_connections].fd);
                        //set up a new epoll event for client messages
                        struct epoll_event client_event;
                        client_event.events = EPOLLIN;
                        client_event.data.fd = ClientList[active_connections].fd;
                        if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, ClientList[active_connections].fd, &client_event) == -1)
                        {
                            perror("epoll_ctl: listen_sock");
                            exit(EXIT_FAILURE);
                        }
                        active_connections++;
                    }
                    
                }
                else{
                    // get messages from connected clients
                    handle_client_messages(events[i].data.fd);
                }
            }
        }

    }

    
    if(TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("close");
    if(TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}
