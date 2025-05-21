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

const char* greek_cities[CITIES_COUNT] = {
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

void print_ownership_table(int ownership_table[CITIES_COUNT]){
    printf("-----STATS-----\n");
    for(int i = 0; i < CITIES_COUNT; i++){
        if(ownership_table[i] == 1){
            printf("%s is a territory of Persian Kingdom\n", greek_cities[i]);
        }
        else{
            printf("%s is a territory of Ancient Greece\n", greek_cities[i]);
        }
    }
}

void shell_wait(int fd, int ownership_table[CITIES_COUNT]){
    char full_line[6];
    while(1){
        printf("Welcome, enter a command\n");
        fflush(stdout);
        //printf(">");
        //fetch the message
        if(fgets(full_line, 6, stdin) == NULL){
            ERR("fgets");
        }
        // if exit
        if(full_line[0] == 'e'){
            if(TEMP_FAILURE_RETRY(close(fd)) < 0)
                ERR("close");
            return;
        }
        // if message
        if(full_line[0] == 'm' && full_line[1] == ' '){
            char msg[MESS_SIZE];
            // if(fgets(msg, MESS_SIZE, stdin) == NULL){
            //     if(TEMP_FAILURE_RETRY(close(fd)) < 0)
            //         ERR("close");
            //     ERR("fgets");
            // }
            msg[0] = full_line[2];
            msg[1] = full_line[3];
            msg[2] = full_line[4];
            full_line[5] = '\0';
            msg[3] = '\n';
            if(bulk_write(fd, msg, MESS_SIZE) < 0){
                ERR("write");
            }

            int city_id;
            int tens = msg[1] - '0';
            int ones = msg[2] - '0';
            city_id = tens*10 + ones;
            if(msg[0] == 'p'){
                ownership_table[city_id-1] = 1;
            }
            else{
                ownership_table[city_id-1] = 0;
            }
        }

        // if travel
        if(full_line[0] == 't' && full_line[1] == ' '){
            char msg[MESS_SIZE];
            if(rand()%2 == 0){
                msg[0] = 'g';
            }
            else{
                msg[0] = 'p';
            }
            msg[1] = full_line[2];
            msg[2] = full_line[3];
            full_line[4] = '\0';
            msg[3] = '\n';

            if(bulk_write(fd, msg, MESS_SIZE) < 0){
                ERR("write");
            }

            int city_id;
            int tens = msg[1] - '0';
            int ones = msg[2] - '0';
            city_id = tens*10 + ones;
            if(msg[0] == 'p'){
                ownership_table[city_id-1] = 1;
            }
            else{
                ownership_table[city_id-1] = 0;
            }
        }
        
        // if print
        if(full_line[0] == 'o'){
            print_ownership_table(ownership_table);
        }
    }
}

int main(int argc, char **argv)
{
    if(argc != 3){
        usage(argv[0]);
    }

    int ownership_table[CITIES_COUNT] = {0};
    // assume all are greek initially

    int sucket = connect_tcp_socket(argv[1], argv[2]);

    shell_wait(sucket, ownership_table);

    // close the connection
    // if(TEMP_FAILURE_RETRY(close(sucket)) < 0){
    //     ERR("close");
    // }
    return 0;
}

// https://github.com/polatoja/OPS/tree/main/sop4
