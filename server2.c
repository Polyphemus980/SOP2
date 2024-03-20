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

sig_atomic_t run=1;
struct message {
    int number;
    pid_t pid;
};

typedef struct message message;

struct threadargs{
    mqd_t* queue;
    int divisor;
};

typedef struct threadargs threadargs;


void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags=SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        perror("sigaction");
}
void handler(int sig, siginfo_t *info, void *context) {

    sleep(10);
     threadargs* args=(threadargs*) info->si_value.sival_ptr;
     mqd_t* queue;
    int divisor;
    queue=args->queue;
    divisor=args->divisor;

    struct sigevent not ;
    not.sigev_notify=SIGEV_SIGNAL;
    not.sigev_signo=SIGRTMIN;
    not .sigev_value.sival_ptr = args;
    int result;
    message msg;
    if (mq_notify(*queue, &not ) < 0)
       perror("mq_notify");
    if (mq_receive(*queue, (char *)&msg, sizeof(message), NULL) < 1)
        perror("mq_receive");
    result = msg.number % divisor;
    char buf[16];
    snprintf(buf,sizeof(buf),"/%d",msg.pid);
    mqd_t client;
    if ( (client=mq_open(buf,O_WRONLY))==-1)
        ERR("mq_open");
    if (mq_send(client,(char*)&result,sizeof(result),10)==-1)
        ERR("mq_send");
    return;
}


void function(union sigval sv)
{
    threadargs* args=(threadargs*) sv.sival_ptr;
    mqd_t* queue;
    int divisor;
    queue=args->queue;
    divisor=args->divisor;
    struct sigevent not ;
    not.sigev_notify=SIGEV_THREAD;
    not.sigev_notify_function=function;
    not.sigev_notify_attributes = NULL;
    not .sigev_value.sival_ptr = args;
    int result;
    message message;
    if (mq_notify(*queue, &not ) < 0)
       perror("mq_notify");
    if (mq_receive(*queue, (char *)&message, sizeof(message), NULL) < 1)
        perror("mq_receive");
    result = message.number % divisor;
    char buf[16];
    snprintf(buf,sizeof(buf),"/%d",message.pid);
    mqd_t client;
    if ( (client=mq_open(buf,O_WRONLY))==-1)
        ERR("mq_open");
    if (mq_send(client,(char*)&result,sizeof(result),10)==-1)
        ERR("mq_send");
    return;
}
int main(int argc,char** argv)
{
    if (argc != 5)
    {
        fprintf(stderr,"Zla liczba argumentow \n");
        return EXIT_FAILURE;
    }
    sethandler(handler,SIGRTMIN);
    struct mq_attr attr={.mq_maxmsg=10,.mq_msgsize=sizeof(message)};
    pid_t pid=getpid();
    int divisors[4];
    mqd_t queue_descriptors[4];
    for (int i=0;i<4;i++)
        divisors[i]=atoi(argv[i+1]);
    char queue_names[4][16];
    threadargs args[4];
    struct sigevent not;
    memset(&not, 0, sizeof(not));
    not.sigev_notify = SIGEV_SIGNAL;
    not.sigev_signo=SIGRTMIN;
    //not.sigev_notify_attributes = NULL;

    for (int i=0;i<4;i++)
    {
        divisors[i]=atoi(argv[i+1]);
        snprintf(queue_names[i],sizeof(queue_names[i]),"/%d_%d",pid,divisors[i]);
        printf("%s \n",queue_names[i]);
        if ((queue_descriptors[i]=mq_open(queue_names[i],O_RDONLY | O_CREAT | O_EXCL , 0666,&attr))==(mqd_t)-1)
            ERR("mq_open");
        args[i].divisor=divisors[i];
        args[i].queue=&queue_descriptors[i];
        not.sigev_value.sival_ptr=&args[i];
         if (mq_notify(queue_descriptors[i], &not) < 0) 
         {
            perror("mq_notify()");
            exit(1);
         }
    }
    sigset_t mask;
    int signo;
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT); 
    sigprocmask(SIG_BLOCK,&mask,NULL);
    while (1){
        sigwait(&mask,&signo);
        if (signo==SIGINT)
           break;
    }

    for (int i=0;i<4;i++)
    {
        mq_close(queue_descriptors[i]);
        mq_unlink(queue_names[i]);
    }
    return EXIT_SUCCESS;

}