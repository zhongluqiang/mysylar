#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#define MAX_EVENTS 10

int main() {
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    assert(timerfd > 0);
    // fcntl(timerfd, F_SETFL, O_NONBLOCK);

    struct itimerspec ts  = {0};
    ts.it_value.tv_sec    = 3; //三秒后超时
    ts.it_interval.tv_sec = 1; //超时后每1秒超时1次
    timerfd_settime(timerfd, 0, &ts, nullptr);

    int epfd = epoll_create1(0);
    assert(epfd > 0);
    epoll_event ev, events[MAX_EVENTS];

    ev.events  = EPOLLIN | EPOLLET;
    ev.data.fd = timerfd;

    int rt = epoll_ctl(epfd, EPOLL_CTL_ADD, timerfd, &ev);
    assert(rt == 0);

    while (1) {
        rt = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (rt < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < rt; i++) {
            if (events[i].data.fd == timerfd) {
                printf("timer expires\n");

                //使用EPOLLET触发方式，一旦有数据，必须全部读干净，否则无法再次触发
                uint64_t buf;
                while (read(timerfd, &buf, sizeof(buf)) > 0)
                    ;
            }
        }
    }

    return 0;
}