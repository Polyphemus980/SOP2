#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

struct message {
    int number;
    pid_t pid;
};

typedef struct message message;

int main(int argc,char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr,"Zla liczba argumentow \n");
        return EXIT_FAILURE;
    }
    char* line=NULL;
    size_t len;
    struct mq_attr attr={.mq_maxmsg=10,.mq_msgsize=sizeof(message)};
    pid_t pid=getpid();
    char* server_name=argv[1];
    mqd_t server_queue,queue;
    char buf[16];
    snprintf(buf,sizeof(buf),"/%d",pid);
    message message;
    if ((queue=mq_open(buf,O_RDONLY | O_CREAT | O_EXCL,0666,&attr))==(mqd_t)-1)
        ERR("mq_open");
    if ((server_queue=mq_open(server_name,O_WRONLY))==(mqd_t)-1)
        ERR("mq_open");
    while (1)
    {
    ssize_t read = getline(&line, &len, stdin);
    if (read == -1) {
            if (feof(stdin)) { // End of input stream
                //free(line);
                break;
            } else {
                ERR("getline");
            }
        }
    message.number=atoi(line);
    message.pid=pid;
    if (mq_send(server_queue,(char*)&message,sizeof(message),10)==-1)
        ERR("mq_send");
    int result;
    if (mq_receive(queue,(char*)&result,sizeof(message),NULL)==-1)
        ERR("mq_receive");
    printf("%d \n",result);
    }

    mq_close(server_queue);
    mq_close(queue);
    mq_unlink(buf);
}