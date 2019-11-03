#include "sylar/sylar.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

//使用协程调度器的IO协程调度功能实现并发TCP ECHO服务器

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

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

void handle_client(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "handle_client begin";
    SYLAR_ASSERT(arg != nullptr);

    //从参数中获取当前的fd上下文
    sylar::FdContext *fd_ctx = (sylar::FdContext *)arg;

    //从fd上下文中获取当前fd
    int clientfd = fd_ctx->fd;
    SYLAR_LOG_INFO(g_logger) << "clientfd=" << clientfd;

    //从fd上下文中获取当前fd已触发的事件
    uint32_t events = fd_ctx->event.events;

    //判断事件类型
    if (events & EPOLLRDHUP) {
        //对端断开连接，从fd上下文中获取协程调度器，并将对应的fd从协程调度中删除
        SYLAR_LOG_INFO(g_logger) << "client(" << clientfd << ")"
                                 << " disconnected.";
        sylar::Scheduler *psc = fd_ctx->scheduler;
        SYLAR_ASSERT(psc != nullptr);
        psc->io_schedule(clientfd, EPOLL_CTL_DEL, 0, nullptr);
        close(clientfd);
    } else if (events & EPOLLIN) {
        //套接字可读
        SYLAR_LOG_INFO(g_logger) << "client(" << clientfd << ") data incoming.";
        char buffer[1024 + 1] = {0};
        int nbytes            = 0;
        while ((nbytes = read(clientfd, buffer, 1024)) > 0) {
            buffer[nbytes] = '\0';
            SYLAR_LOG_INFO(g_logger)
                << "read from client(" << clientfd << "):" << buffer;
        }
    }

    SYLAR_LOG_INFO(g_logger) << "handle_client end";
}

void do_accept(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "do_accept begin";
    SYLAR_ASSERT(arg != nullptr);

    //从参数中获取当前的fd上下文
    sylar::FdContext *fd_ctx = (sylar::FdContext *)arg;

    //从fd上下文中获取当前fd
    int serverfd = fd_ctx->fd;
    SYLAR_LOG_INFO(g_logger) << "serverfd=" << serverfd;

    //从fd上下文中获取当前fd已触发的事件
    uint32_t events = fd_ctx->event.events;

    //判断事件类型
    if (events & EPOLLIN) {
        //接收客户端，获取客户端套接字fd
        int clientfd = accept(serverfd, NULL, NULL);
        SYLAR_LOG_INFO(g_logger) << "new clientfd=" << clientfd;
        SYLAR_ASSERT(clientfd > 0);

        //从fd上下文中获取当前协程调度器
        sylar::Scheduler *psc = fd_ctx->scheduler;
        SYLAR_ASSERT(psc != nullptr);

        //为客户端套接字fd添加IO调度
        SYLAR_LOG_INFO(g_logger) << "add fd(" << clientfd << ") to scheduler.";
        psc->io_schedule(clientfd, EPOLL_CTL_ADD,
                         EPOLLIN | EPOLLET | EPOLLRDHUP, &handle_client);
    } else {
        SYLAR_LOG_ERROR(g_logger) << "unknown events:" << events;
    }

    SYLAR_LOG_INFO(g_logger) << "do_accept end";
}

int timerfd;
void test_io_fiber_schedule(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_io_fiber_schedule begin";
    SYLAR_ASSERT(arg != nullptr);
    sylar::Scheduler *psc = (sylar::Scheduler *)arg;
    psc->io_schedule(timerfd, EPOLL_CTL_DEL, 0, nullptr);
    close(timerfd);
    SYLAR_LOG_INFO(g_logger) << "test_io_fiber_schedule end";
}

void test_io_schedule(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_io_schedule begin";
    SYLAR_ASSERT(arg != nullptr);

    int serverfd = create_tcp_server_socket(9999);
    SYLAR_ASSERT(serverfd > 0);
    //所有参与IO调度的fd会被自动设为非阻塞模式，这一步可跳过
    // fcntl(serverfd, F_SETFL, O_NONBLOCK);
    SYLAR_LOG_INFO(g_logger) << "server create, serverfd=" << serverfd;

    sylar::Scheduler *psc = (sylar::Scheduler *)arg;

    //为服务器监听套接字fd添加IO协程调度任务
    psc->io_schedule(serverfd, EPOLL_CTL_ADD, EPOLLIN | EPOLLET, &do_accept);

    //增加一个timerfd，用于测试协程对象的调度
    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    SYLAR_ASSERT(timerfd > 0);
    struct itimerspec ts = {0};
    ts.it_value.tv_sec   = 3; //三秒后超时
    timerfd_settime(timerfd, 0, &ts, nullptr);
    sylar::Fiber::ptr fiber(new sylar::Fiber(test_io_fiber_schedule, psc));
    psc->io_schedule(timerfd, EPOLL_CTL_ADD, EPOLLIN | EPOLLET, fiber);

    SYLAR_LOG_INFO(g_logger) << "test_io_schedule end";
}

int main() {
    SYLAR_LOG_INFO(g_logger) << "main begin";

    sylar::Scheduler sc;
    sc.schedule(&test_io_schedule, &sc);
    sc.start();

    // do not stop
    for (;;) {
        sleep(1);
    }

    SYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}