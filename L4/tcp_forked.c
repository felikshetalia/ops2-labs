/**
 * \brief TCP example - parent process sends data to the child process.
 */

 #include <unistd.h>
 #include <string.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <sys/socket.h>
 #include <sys/wait.h>
 #include <netdb.h>
 #include <arpa/inet.h>
 
 #define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
 
 struct sockaddr_in make_address(const char *address, const char *port) {
     int ret;
     struct sockaddr_in addr;
     struct addrinfo *result;
     struct addrinfo hints = {};
     hints.ai_family = AF_INET;
     if ((ret = getaddrinfo(address, port, &hints, &result))) {
         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
         exit(EXIT_FAILURE);
     }
     addr = *(struct sockaddr_in *) (result->ai_addr);
     freeaddrinfo(result);
     return addr;
 }
 /*
     TCP sockets schema:
 
     Server: socket() -> setsockopt() tcp related flag -> bind() -> listen() -> accept() -> send/recv()
     Client: socket() -------------------> connect() -------------------------------------> send/recv()
 */
 void child_work(struct sockaddr_in addr){
     //clients
     int cliSock = socket(AF_INET, SOCK_STREAM, 0);
     if(cliSock < 0)
         ERR("sock");
 
     int opt = 1;
     if(setsockopt(cliSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
         ERR("setsockopt");
     }
     if(connect(cliSock, (const struct sockaddr*)&addr, sizeof(addr)) < 0){
         ERR("connect");
     }
     char buffer[50];
     ssize_t ret;
     if((ret = recv(cliSock, buffer, sizeof(buffer), 0)) < 0){
         ERR("recv");
     }
     printf("Streets say: %s of size %ld\n", buffer, ret);
     if(close(cliSock) < 0){
         ERR("close");
     }
 
 }
 
 void parent_work(struct sockaddr_in addr){
     // the server
     int serverSock = socket(AF_INET, SOCK_STREAM, 0);
     if(serverSock < 0)
         ERR("sock");
 
     int opt = 1;
     if(setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
         ERR("setsockopt");
     }
     if(bind(serverSock, (const struct sockaddr*)&addr, sizeof(addr)) < 0){
         ERR("bind");
     }
     if(listen(serverSock, 3) < 0){
         ERR("listen");
     }
     struct sockaddr clientAddr;
     socklen_t len = sizeof(clientAddr);
     int acceptedSock = accept(serverSock, &clientAddr, &len); //to accept you always need a clientaddr (sockaddr)
     if(acceptedSock < 0){
         ERR("accept");
     }
     //char buf[256];
     char msg[] = "are y'all ready kids?";
    if(send(acceptedSock, msg, sizeof(msg), 0) < 0){
        ERR("send");
    }
     if(close(acceptedSock) < 0)
         ERR("close");
     if(close(serverSock) < 0)
         ERR("close");
 }
 
 void create_children(struct sockaddr_in addr){
     switch(fork()){
         case -1:
             ERR("fork");
             exit(EXIT_FAILURE);
         case 0:
             child_work(addr);
             exit(EXIT_SUCCESS);
         default:
             parent_work(addr);
             while(wait(NULL) > 0);
             exit(EXIT_SUCCESS);
     }
 }
 
 int main(int argc, char **argv){
     if (argc != 3) {
         fprintf(stderr, "USAGE: %s host port\n", argv[0]);
         exit(EXIT_FAILURE);
     }
 
     struct sockaddr_in addr = make_address(argv[1], argv[2]);
     create_children(addr);
 
     return EXIT_SUCCESS;
 }