#include "common.h"

#define MAX_CLIENTS 4
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

int main(int argc, char** argv){
    if(argc != 2){
        usage(argv[0]);
    }
    int port = atoi(argv[1]);
    if(port < 1024){
        usage(argv[0]);
    }

    int server_fd = bind_tcp_socket(port, BACKLOG);

    char input[4];
    // 1st stage is a single client. no epoll needed

    int accepted_client = add_new_client(server_fd);

        int ret = bulk_read(accepted_client, input, 4);
        if(ret < 0)
            ERR("read");
        if(ret == 0){
            if(TEMP_FAILURE_RETRY(close(accepted_client)) < 0)
                ERR("close");

        }
        if(ret > 0){
            printf("%s\n", input);
        }

    
    if(TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}
