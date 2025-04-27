#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>

#define MAXLENGTH 100

#define LIFE_SPAN 10
#define MAX_NUM 10

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))



void usage(void){
	fprintf(stderr,"USAGE: prog1_stage1 q0_name \n");	
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) 
{
	if(argc!=2) usage();

	struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAXLENGTH;
    attr.mq_curmsgs = 0;
	mqd_t q0;
	char q0_name[MAXLENGTH];
	snprintf(q0_name, MAXLENGTH, "%s", argv[1]);
	if((q0 = mq_open(q0_name, O_RDWR, 0600, &attr)) < 0){
        ERR("mq_open");
    }
	char msg[MAXLENGTH];
	snprintf(msg, MAXLENGTH, "register %d", getpid()); //format
	if(mq_send(q0, msg, strlen(msg)+1, 0) < 0){
        ERR("mq_send");
    }

	mq_close(q0);
	//mq_unlink(q0_name);
	return EXIT_SUCCESS;
}