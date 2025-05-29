#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))
#define FIFO_NAME "./myfifo"

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

const char *paths[3] = {
    "/home/cansu/OPS/L1/1.txt",
    "/home/cansu/OPS/L1/2.txt",
    "/home/cansu/OPS/L1/3.txt"
};

void read_from_file(){
    for (int i = 0; i < 3; i++) {
        printf("File path %d: %s\n", i + 1, paths[i]);
    }
    FILE *files[3];
    char buffer[1024];
    for(int i = 0; i < 3; i++){
        files[i] = fopen(paths[i], "r");
        if(!files[i]){
            ERR("fopen");
        }
        while (fgets(buffer, sizeof(buffer), files[i])) {
            printf("%s", buffer); // Print each line from the file
        }
        printf("\n");
    }
    for(int i = 0; i < 3; i++){
        if(fclose(files[i]) < 0){
            ERR("close");
        }
    }

}

void write_to_fifo(int fd, int txtfile){
    int64_t ret;
    char buffer[PIPE_BUF];
    char *bit;
    *((pid_t *)buffer) = getpid();
    bit = buffer + sizeof(pid_t);

    do{
        if((ret = read(txtfile,bit,MSG_SIZE)) < 0){
            ERR("read");
        }
        if(ret < MSG_SIZE){
            memset(bit+ret, 0, MSG_SIZE-ret);
        }
        if(ret > 0){
            if(write(fd,buffer,PIPE_BUF) < 0){
                ERR("write");
            }
        }
    }while(ret == MSG_SIZE);
}

const char *fifos[3] = {
    "/tmp/fifo1",
    "/tmp/fifo2",
    "/tmp/fifo3"
};

void read_txt_int_descriptor(int txt[3], char *buffer){
    for (int i = 0; i < 3; i++) {
        printf("File path %d: %s\n", i + 1, paths[i]);
    }
    //int txt[3];
    //char buffer[1024];
    for(int i = 0; i < 3; i++){
        if((txt[i] = open(paths[i], O_RDONLY)) < 0){
            ERR("open");
        }
        ssize_t ret;
        if((ret = read(txt[i], buffer, sizeof(buffer))) < 0){
            ERR("read");
        } else {
            //buffer[i][ret] = '\0';
            printf("%s", buffer);
        }
        printf("\n");
        close(txt[i]);
    }
}

void with_single_msg(){
    int fd, txt;
    char buf[2048];
    if(mkfifo(fifos[0], 0666) < 0){
        if(errno != EEXIST){
            ERR("mkfifo");
        }
    }
    if((txt = open(paths[0], O_RDONLY)) < 0){
        ERR("open");
    }
    if(read(txt, buf, sizeof(buf)) < 0){
        ERR("read");
    }
    //printf("%s\n",buf);
    close(txt);
    
    pid_t pid = fork();
    switch (pid)
    {
    case -1:
        perror("fork(): ");
        return;
    case 0:
        //child
        if((fd = open(fifos[0], O_WRONLY)) < 0){
            ERR("open");
        }
        if(write(fd, buf, sizeof(buf)) < 0){
            ERR("write");
        }
        close(fd);
        break;
    default:
        // parent
        char wiadomosc[2048];
        ssize_t ret;
        if((fd = open(fifos[0], O_RDONLY)) < 0){
            ERR("open");
        }
        if((ret = read(fd, wiadomosc, sizeof(wiadomosc))) < 0){
            ERR("read");
        }
        else{
            printf("Total bytes read: %ld\n", ret);
            printf("%s\n", wiadomosc);
        }
        close(fd);
        break;
    }
    unlink(fifos[0]);
}

int main(int argc, char** argv){
    int fd[3], txt[3];
    char buf[2048];
    pid_t pid;
    for(int i = 0; i < 3; i++){
        if(mkfifo(fifos[i], 0666) < 0){
            if(errno != EEXIST){
                ERR("mkfifo");
            }
        }
        if((txt[i] = open(paths[i], O_RDONLY)) < 0){
            ERR("open");
        }
        if(read(txt[i], buf, sizeof(buf)) < 0){
            ERR("read");
        }
        //printf("%s\n",buf);
        close(txt[i]);
    }
    for(int i = 0; i < 3; i++){
        pid = fork();
        switch (pid)
        {
        case -1:
            perror("fork(): ");
            return 1;
        case 0:
            //child
            if((fd[i] = open(fifos[i], O_WRONLY)) < 0){
                ERR("open");
            }
            if(write(fd[i], buf, sizeof(buf)) < 0){
                ERR("write");
            }
            close(fd[i]);
            break;
        default:
            // parent
            char wiadomosc[2048];
            ssize_t ret;
            if((fd[i] = open(fifos[i], O_RDONLY)) < 0){
                ERR("open");
            }
            if((ret = read(fd[i], wiadomosc, sizeof(wiadomosc))) < 0){
                ERR("read");
            }
            else{
                printf("Total bytes read: %ld\n", ret);
                printf("%s\n", wiadomosc);
            }
            while(wait(NULL) > 0);
            close(fd[i]);
            break;
        }
    } 
    for(int i = 0; i < 3; i++)
        unlink(fifos[i]);
    
    return 0;
}