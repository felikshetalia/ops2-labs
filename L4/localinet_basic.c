#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))
#endif

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

void server_work(struct sockaddr_un addr)
{
    int ser;
    if((ser = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        ERR("socket");
    }
    int optval;
    if(0 > setsockopt(ser, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))){
        ERR("setsockopt");
    }
    if(0 > bind(ser, (const struct sockaddr*)&addr, sizeof(addr))){
        ERR("bind");
    }
    if(0 > listen(ser, 1)){
        ERR("listen");
    }

    // accept
    struct sockaddr client_addrss;
    // you make an instance of the client address on the server side, client uses the one initially made
    socklen_t len = sizeof(client_addrss);
    int guest = accept(ser, &client_addrss, &len);
    if(0 > guest) ERR("accept");
    char buffer[200];
    while(1){
        
        int ret;
        if((ret = recv(guest, buffer, sizeof(buffer), 0)) < 0){
            ERR("recv");
        }
        if(ret == 0) break;
        if(ret > 0){
            buffer[ret] = '\0';
            printf("CLIENT: %s\n", buffer);
        }
    }
    if(0>close(ser)) ERR("close");
    if(0>close(guest)) ERR("close");
}

void client_work(struct sockaddr_un addr)
{
    sleep(2);
    int cli;
    if((cli = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        ERR("socket");
    }
    if(connect(cli, (const struct sockaddr*)&addr, sizeof(addr)) < 0){
        ERR("connect");
    }

    for(int i = 0; i < 10; i++){
        char msg[20];
        snprintf(msg, 20, "bulk %d", i+1);
        if(send(cli, msg, strlen(msg)+1, 0) < 0){
            ERR("send");
        }
        sleep(1);
    }
    if(0>close(cli)) ERR("close");
}

void create_children(struct sockaddr_un addr){
    switch(fork()){
        case -1:
            ERR("fork");
            exit(EXIT_FAILURE);
        case 0:
            client_work(addr);
            exit(EXIT_SUCCESS);
        default:
            server_work(addr);
            while(wait(NULL) > 0);
            exit(EXIT_SUCCESS);
    }
}

int main(int argc, char** argv)
{
    if(argc != 2){
        // do it with /tmp/name to not get "permission denied"
        printf("Provide a path\n");
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = PF_UNIX;
    strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path) - 1);

    create_children(addr);

    if(unlink(argv[1]) < 0){
        ERR("unlink");
    }
    return 0;
}