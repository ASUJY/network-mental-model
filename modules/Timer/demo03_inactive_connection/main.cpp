/**
 * 处理非活动连接
 *
 * 利用alarm函数周期性地触发SIGALRM信号，该信号的信号处理函数利用
 * 管道通知主循环执行定时器链表上的定时任务——关闭非活动的连接
 */

#include "lst_timer.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 5

static int pipefd[2];
// 利用升序链表来管理定时器
static sort_timer_lst timer_lst;
static int epollfd = 0;

// 将文件描述符设置为非阻塞的
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将文件描述符fd上的EPOLLIN注册到epollfd指示的epoll内核事件表中
int addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void timer_handler() {
    // 定时处理任务，实际上就是调用tick函数
    timer_lst.tick();
    // 因为一次alarm调用只会引起一次SIGALRM信号，所以我们要重新定时，以不断
    // 触发SIGALRM信号
    alarm(TIMESLOT);
}

// 定时器回调函数，它删除非活动连接socket上的注册事件，并关闭之
void cb_func(client_data *user_data) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    printf("close fd %d\n", user_data->sockfd);
}

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    // 创建服务器socket地址
    sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    // 创建服务器socket文件
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_sockfd >= 0);

    // 绑定服务器socket文件和服务器socket地址
    int ret = bind(server_sockfd, (sockaddr *)&server_address,
                   sizeof(server_address));
    assert(ret != -1);

    // 让内核监听服务器socket文件
    ret = listen(server_sockfd, 5);
    assert(ret != -1);

    // 在内核中创建一个epoll实例
    int epollfd = epoll_create(100);
    assert(epollfd != -1);
    // 创建文件描述符集合
    epoll_event events[MAX_EVENT_NUMBER];
    addfd(epollfd, server_sockfd);

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    // 设置信号处理函数
    addsig(SIGALRM);
    addsig(SIGTERM);
    bool stop_server = false;

    client_data *users = new client_data[FD_LIMIT];
    bool timeout = false;
    alarm(TIMESLOT); // 定时

    while (!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            // 处理新到的客户连接
            if (sockfd == server_sockfd) {
                sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(server_sockfd, (sockaddr *)&client_address,
                                    &client_addrlength);
                addfd(epollfd, connfd);
                users[connfd].address = client_address;
                users[connfd].sockfd = connfd;

                // 创建定时器，设置其回调函数与超时时间，然后绑定定时器与用户数据，
                // 最后将定时器添加到链表timer_lst中
                util_timer *timer = new util_timer;
                timer->user_data = &users[connfd];
                timer->cb_func = cb_func;
                time_t cur = time(NULL);
                timer->expire = cur + 3 * TIMESLOT;
                users[connfd].timer = timer;
                timer_lst.add_timer(timer);
            } else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
                // 处理信号
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    // handle the error
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                        case SIGALRM: {
                            // 用timeout变量标记有定时任务需要处理，但不立即处理定时任务。
                            // 这是因为定时任务的优先级不是很高，我们优先处理其他更重要的任务
                            timeout = true;
                            break;
                        }
                        case SIGTERM: {
                            stop_server = true;
                        }
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                // 处理客户连接上接收到的数据
                memset(users[sockfd].buf, '\0', BUFFER_SIZE);
                ret = recv(sockfd, users[sockfd].buf, BUFFER_SIZE - 1, 0);
                printf("get %d bytes of client data %s from %d\n", ret,
                       users[sockfd].buf, sockfd);

                util_timer *timer = users[sockfd].timer;
                if (ret < 0) {
                    // 如果发生读错误，则关闭连接，并移除其对应的定时器
                    if (errno != EAGAIN) {
                        cb_func(&users[sockfd]);
                        if (timer) {
                            timer_lst.del_timer(timer);
                        }
                    }
                } else if (ret == 0) {
                    // 如果对方已经关闭连接，则我们也关闭连接，并移除对应的定时器
                    cb_func(&users[sockfd]);
                    if (timer) {
                        timer_lst.del_timer(timer);
                    }
                } else {
                    // 如果某个客户连接上有数据可读，则我们要调整该连接对应的定时器，以
                    // 延迟该连接被关闭的时间
                    if (timer) {
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        printf("adjust timr once\n");
                        timer_lst.adjust_timer(timer);
                    }
                }
            } else {
                // other
            }
        }

        // 最后处理定时事件，因为I/O事件有更高的优先级。当然，这样做将导致定时任务不能精确地
        // 按照预期的时间执行。
        if (timeout) {
            timer_handler();
            timeout = false;
        }
    }

    close(server_sockfd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] users;

    return 0;
}
