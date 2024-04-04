#define _GNU_SOURCE

#include "wait.h"
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "semaphore.h"
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define SHM_SIZE 1024
#define SHM_NAME "/shm-name"

typedef struct player_data{
    int card;
    int points;
} player_data;

typedef struct table{
    sem_t semaphor;
    pthread_barrier_t barrier;
    player_data* data;
}table;


void child_work(table* table,player_data* data,int index,int M)
{
    int card;
    for (int i=0;i<M;i++)
    {   
        card=rand() % M;
        data[index].card=card;
       pthread_barrier_wait(&(table->barrier));

       pthread_barrier_wait(&(table->barrier));
       printf("[%d] player : Round [%d] POINTS: [%d] \n",index,i,data[index].points);
       pthread_barrier_wait(&(table->barrier));
    }
}
void parent_work(table* table,player_data* data,int N,int M)
{
    int max=0;
    for (int i=0;i<N;i++)
    {
        data[i].points=0;
    }
    for (int i=0;i<M;i++)
    {
        pthread_barrier_wait(&(table->barrier));
        for (int j=0;j<N;j++)
        {
            if (data[j].card > data[max].card)
                max=j;
        }
        data[max].points+=N;
        pthread_barrier_wait(&(table->barrier));
        pthread_barrier_wait(&(table->barrier));
    }
}
int main(int argc,char** argv){

    int N=atoi(argv[1]);
    int M=atoi(argv[2]);

    int shm_id=shm_open(SHM_NAME,O_CREAT | O_RDWR | O_EXCL,0666 );
    if (shm_id==-1)
        ERR("shm_open");

    if (ftruncate(shm_id,SHM_SIZE)==-1)
        ERR("ftruncate");

    table* table;
    table=mmap(NULL,sizeof(table),PROT_WRITE | PROT_READ, MAP_SHARED,shm_id,0);
    if (table==MAP_FAILED)
    {
        ERR("mmap");
    }
    player_data* data= (player_data*)table+ sizeof(pthread_barrier_t);
    pthread_barrierattr_t attr;
    pthread_barrierattr_init(&attr);
    pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&table->barrier, &attr, N+1);
    
    for (int i=0;i<N;i++)
    {
        switch (fork())
        {
            case -1:ERR("fork");
            case 0:child_work(table,data,i,M);exit(EXIT_SUCCESS);
        }
    }
    parent_work(table,data,N,M);


    while (wait(NULL)>0);

}