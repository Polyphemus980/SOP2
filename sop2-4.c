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
sig_atomic_t run=1;
#define BACKLOG_SIZE 10
#define MAX_CLIENT_COUNT 4
#define MAX_EVENTS 10

#define NAME_OFFSET 0
#define NAME_SIZE 64
#define MESSAGE_OFFSET NAME_SIZE
#define MESSAGE_SIZE 448
#define BUFF_SIZE (NAME_SIZE + MESSAGE_SIZE)
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
void usage(char *program_name) {
    fprintf(stderr, "USAGE: %s port key\n", program_name);
    exit(EXIT_FAILURE);
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
void siginthandler(int signo){
    run=0;
}
struct serverptr{
    int serverfd;
    int serverid;
};

int validate_client(int client_socket,char* key)
{
    char buf[512];
    char name[NAME_SIZE],client_key[MESSAGE_SIZE];
    //bulk_read(client_socket,buf,sizeof(buf));
    recv(client_socket,buf,sizeof(buf),0);
    memcpy(name,buf,64);
    //name[63]='\0';
    memcpy(client_key,buf+NAME_SIZE,MESSAGE_SIZE);
    //client_key[MESSAGE_SIZE-1]='\0';
    // snprintf(name,NAME_SIZE,"%s",buf);
    // snprintf(client_key,MESSAGE_SIZE,"%s",buf+MESSAGE_OFFSET);
    printf("%s : %s\n",name,client_key);
    if (!strcmp(client_key,key)==0)
    {    
        close(client_socket);
        printf("Invalid key \n");
        return -1;
    }
    bulk_write(client_socket,buf,sizeof(buf));
    return 1;
}
void close_client(int client_socket, int* clients, int* clients_count, int epoll_descriptor) {
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_DEL, client_socket, NULL) == -1) {
        perror("epoll_ctl: client_sock");
        exit(EXIT_FAILURE);
    }
    printf("Closing client \n");
    if (close(client_socket) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
        if (clients[i] == client_socket) {
            clients[i] = -1;
            (*clients_count)--;
            break;
        }
    }
}
void doServer(int tcp_listen_socket,char* key)
{
    int clients[MAX_CLIENT_COUNT];
    int clients_count=0;
    for (int i=0;i<MAX_CLIENT_COUNT;i++)
    {
        clients[i]=-1;
    }
    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd=tcp_listen_socket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, tcp_listen_socket, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    int nfds;
    sigset_t mask,oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while (run)
    {
        if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, 30000,&oldmask)) > 0)
        {
            for (int i=0;i<nfds;i++)
            {
                if (events[i].data.fd == tcp_listen_socket)
                {
                    int client_socket = add_new_client(tcp_listen_socket);
                    if (client_socket==-1)
                        continue;
                    clients_count++;
                    if (clients_count==MAX_CLIENT_COUNT)
                    {
                        close(client_socket);
                        printf("Max users \n");
                        continue;
                    }
                    if (validate_client(client_socket,key)==-1)
                    {
                        continue;
                    }
                    else
                    {
                        event.data.fd=client_socket;
                        if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_socket, &event) == -1)
                        {
                            perror("epoll_ctl: client_sock");
                            exit(EXIT_FAILURE);
                            
                        }
                        printf("%d \n",clients_count);
                    }
                    for (int i=0;i<MAX_CLIENT_COUNT;i++)
                    {
                        if (clients[i] == -1)
                        {
                            clients[i] = client_socket;
                            break;
                        }  
                    }
                }
                else
                {
                    char buf[512],name[64],message[MESSAGE_SIZE];
                    int client_socket=events[i].data.fd;
                    ssize_t rec,sen;
                    if ((rec=recv(client_socket,buf,sizeof(buf),0))<=0)
                    {
                        if (errno == EPIPE || errno == ECONNREFUSED || rec == 0)
                        {
                            close_client(client_socket,clients,&clients_count,epoll_descriptor);
                        }
                        continue;
                        printf("koks \n");
                    }
                    memcpy(name,buf,64);
                    name[63]='\0';
                    memcpy(message,buf+NAME_SIZE,MESSAGE_SIZE);
                    message[MESSAGE_SIZE-1]='\0';
                    //snprintf(name,(unsigned long long) NAME_SIZE,"%s",buf);
                    //snprintf(message,(unsigned long long) MESSAGE_SIZE,"%s",buf+MESSAGE_OFFSET);
                    printf("%s : %s\n",name,message);
                    for (int i=0;i<MAX_CLIENT_COUNT;i++)
                    {
                        if (clients[i] != client_socket && clients[i] != -1)
                        {
                            if ((sen=send(clients[i],buf,sizeof(buf),0))<0)
                            {
                                if (errno == EPIPE || errno == ECONNREFUSED)
                                {
                                    close_client(clients[i],clients,&clients_count,epoll_descriptor);
                                }
                            }
                        }
                    }
                }
                
            }
        }
    }
    for (int i=0;i<clients_count;i++)
    {
        if (clients[i] != -1)
            close(clients[i]);
    }
    printf("Server closes \n");
    if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
    return;
}
int main(int argc,char** argv)
{
    char *program_name = argv[0];
    if (argc != 3) {
        usage(program_name);
    }

    uint16_t port = atoi(argv[1]);
    if (port == 0){
        usage(argv[0]);
    }
    sethandler(siginthandler,SIGINT);
    char *key = argv[2];
    int sockfd=bind_tcp_socket(port,10);
    doServer(sockfd,key);
    return 0;
}

