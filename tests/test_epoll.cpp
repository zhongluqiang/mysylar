#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

/*
- 接口：
    1. int epoll_create(int size);
    创建epoll实例，返回对应的文件描述符，使用结束后需要用close()来关闭，size参
    数可忽略，只需要大于0即可

    2. int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    epoll控制接口，用于添加/删除/修改epoll监视的描述符及其事件，参数描述如下：

    操作码op描述：
        EPOLL_CTL_ADD ：添加fd到epoll实例上，其关心的事件为event
        EPOLL_CTL_MOD : 修改fd对应的事件
        EPOLL_CTL_DEL : 删除epoll实例中的fd描述符，event传NULL即可

    事件结构体描述：
    typedef union epoll_data {
        void        *ptr;
        int          fd;
        uint32_t     u32;
        uint64_t     u64;
    } epoll_data_t;

    struct epoll_event {
        uint32_t     events;      //epoll事件
        epoll_data_t data;        //epoll用户数据
    };

    事件描述：
        EPOLLIN  : fd可读事件
        EPOLLOUT : fd可写事件
        EPOLLERR : fd发生错误事件，epoll_wait总是监听该事件，无需手动添加
        EPOLLRDHUP : 套接字对端关闭连接，或者关闭了写半边
        EPOLLHUP : 对端挂起事件，例如管道/套接写写端关闭时，读端将收到该事件
        EPOLLET  : 设置以上事件类型为边缘触发模式
        EPOLLONESHOT : 设置以上事件只触发一次

    3. int epoll_wait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout);
     epoll等待事件接口，等待epoll实例内注册的文件描述符的相关事件是否发生。当事
     件发生时，events数组保存了对应的事件结构体，从中可以获取哪些描述符发生了哪
     些事件。maxevents用于指定返回的最大事件数。timeout用于指定待超时，单位毫
     秒，当为-1时表示永久等待，当为0时epoll会立即返回。epoll会在以下情况返回：
        1. 某个文件描述符触发了事件；
        2. epoll_wait调用被信号中断；
        3. timeout超时
        epoll返回触发事件的文件描述符的个数，如果超时仍无事件触发则返回0，出错返
        回-1并设置errno。


- 使用边缘触发的一般步骤：
    1. 设置文件描述符为非阻塞方式；
    2. 只有读写返回EAGAIN时才重新加入等待

- 指定EPOLLONESHOT标志时，描述符的指定事件只会上报一次，需要再次上报的话要用
  EPOLL_CTL_MOD加回去

- /proc/sys/fs/epoll/max_user_watches用于指定当前系统所有epoll实例能够监听的最大
文件描述符数目

- 出于性能考虑，可以同时添加EPOLLIN | EPOLLOUT事件
*/

#define MAX_EVENTS 10

int create_tcp_server_socket(short port) {
    int sockfd = 0;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main() {
    int epollfd;
    struct epoll_event ev, events[MAX_EVENTS];
    int serverfd, nfds;

    serverfd = create_tcp_server_socket(9999);
    fcntl(serverfd, F_SETFL, O_NONBLOCK);

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events  = EPOLLIN | EPOLLET;
    ev.data.fd = serverfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == serverfd) {
                printf("client incoming\n");
                int clientfd = accept(serverfd, NULL, NULL);
                fcntl(clientfd, F_SETFL, O_NONBLOCK);
                ev.events  = EPOLLIN | EPOLLET | EPOLLRDHUP;
                ev.data.fd = clientfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) == -1) {
                    perror("epoll_ctl");
                    exit(EXIT_FAILURE);
                }
            } else if (
                events[i].events &
                EPOLLRDHUP) { /* 对端关闭时同时返回EPOLLIN和EPOLLRDHUP，
                               *先检测EPOLLRDHUP，保证连接未关闭之后再读取数据
                               */
                printf("peer close connection\n");
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd,
                              NULL) == -1) {
                    perror("epoll_ctl");
                    exit(EXIT_FAILURE);
                }
                close(events[i].data.fd);
            } else if (events[i].events & EPOLLIN) {
                char buffer[1024];
                int nbytes = 0;
                while ((nbytes = read(events[i].data.fd, buffer, 1024)) > 0) {
                    buffer[nbytes] = '\0';
                    printf("%s", buffer);
                }
            }
        }
    }

    return 0;
}