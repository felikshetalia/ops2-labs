#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
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

int make_local_socket(char *name, struct sockaddr_un *addr)
{
    int socketfd;
    if ((socketfd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
        ERR("socket");
    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, name, sizeof(addr->sun_path) - 1);
    return socketfd;
}

int connect_local_socket(char *name)
{
    struct sockaddr_un addr;
    int socketfd;
    socketfd = make_local_socket(name, &addr);
    if (connect(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0)
    {
        ERR("connect");
    }
    return socketfd;
}

int bind_local_socket(char *name, int backlog_size)
{
    struct sockaddr_un addr;
    int socketfd;
    if (unlink(name) < 0 && errno != ENOENT)
        ERR("unlink");
    socketfd = make_local_socket(name, &addr);
    if (bind(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0)
        ERR("bind");
    if (listen(socketfd, backlog_size) < 0)
        ERR("listen");
    return socketfd;
}

int make_tcp_socket(void)
{
    int sock;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        ERR("socket");
    return sock;
}

struct sockaddr_in make_address(char *address, char *port)
{
    int ret;
    struct sockaddr_in addr;
    struct addrinfo *result;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    if ((ret = getaddrinfo(address, port, &hints, &result)))
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }
    addr = *(struct sockaddr_in *)(result->ai_addr);
    freeaddrinfo(result);
    return addr;
}

int connect_tcp_socket(char *name, char *port)
{
    struct sockaddr_in addr;
    int socketfd;
    socketfd = make_tcp_socket();
    addr = make_address(name, port);
    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        ERR("connect");
    }
    return socketfd;
}

int bind_tcp_socket(uint16_t port, int backlog_size)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;
    socketfd = make_tcp_socket();
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("bind");
    if (listen(socketfd, backlog_size) < 0)
        ERR("listen");
    return socketfd;
}

int add_new_client(int sfd)
{
    int nfd;
    if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return -1;
        ERR("accept");
    }
    return nfd;
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (0 == c)
            return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

int main(int argc, char** argv)
{

    // parent sets up server
    // 1.
    int sock_fd; // server
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        ERR("socket");
    }

    // 2.
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int opt = 1;
    if(0 > setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt))){
        ERR("setsockopt");
    }


    // 2.
    // struct sockaddr_in cliaddr;
    // cliaddr.sin_family = AF_INET;
    // cliaddr.sin_port = htons(atoi(argv[1]));
    // cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // memset(&cliaddr, 0, sizeof(cliaddr));

    // 3. bind!!!
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ERR("bind");
    }
    
    // 4. listen
    if (listen(sock_fd, 1) < 0) {
        ERR("listen");
    }

    pid_t pid = fork();
    if(pid < 0) ERR("fork");
    else if(pid == 0){
        // child
        sleep(1);
        if(0>close(sock_fd)) ERR("close");
        int cli;
        if((cli = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            ERR("socket");
        }
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(atoi(argv[1]));
        serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if(connect(cli, &addr, sizeof(addr)) < 0){
            ERR("failed to connect"); // check up
        }
        char msg[100];
        snprintf(msg, 100, "Child from port %s speaking.\n", argv[1]);
        if(send(cli, (const char*)msg, strlen(msg)+1,0) < 0){
            ERR("sendto");
        }
        if(0>close(cli)) ERR("close");
    }
    else{
        // parent
        char buf[100];
        int ret;
        //socklen_t size = sizeof(cliaddr);
        int client_fd;
        if ((client_fd = accept(sock_fd, NULL, NULL)) < 0) {
            ERR("accept");
        }
        if((ret = recv(client_fd, buf, sizeof(buf), MSG_WAITALL)) < 0){
            ERR("recvfrom");
        }
        //buf[ret] = "\0";
        printf("MSG read successfully: %s\n", buf);
        if(0>close(client_fd)) ERR("close");
    }
    

    return 0;

}

// NOTES:

// 1. make the socket via socket()
// 2. make address


