/**
 * 同时处理TCP请求和UDP请求的回射服务器
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

// 将文件描述符设置为非阻塞
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将文件描述符fd上的EPOLLIN注册到epollfd指示的epoll内核事件表中
void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage %s ip_address port_number\n", basename(argv[0]));
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

    // 创建服务器 TCP socket文件
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_sockfd >= 0);
    // TCP socket文件绑定服务器socket地址
    int ret = bind(server_sockfd, (sockaddr *)&server_address,
                   sizeof(server_address));
    assert(ret != -1);
    // 让内核开始监听socket文件
    ret = listen(server_sockfd, 5);
    assert(ret != -1);

    // 创建UDP socket文件，并将其绑定到指定的端口port上
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    int server_udpsocketfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(server_udpsocketfd >= 0);
    ret = bind(server_udpsocketfd, (sockaddr *)&server_address,
               sizeof(server_address));
    assert(ret != -1);

    // 在内核中创建一个epoll实例
    int epollfd = epoll_create(100);
    assert(epollfd != -1);
    // 往epoll实例中添加需要内核监听的文件描述符
    epoll_event events[MAX_EVENT_NUMBER];
    addfd(epollfd, server_sockfd);
    addfd(epollfd, server_udpsocketfd);

    while (1) {
        // 调用IO复用系统调用函数epoll，让内核监听指定文件描述符是否有读事件发生
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == server_sockfd) {
                sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int client_fd =
                    accept(server_sockfd, (sockaddr *)&client_address,
                           &client_addrlength);
                addfd(epollfd, client_fd);
            } else if (sockfd == server_udpsocketfd) {
                char buf[UDP_BUFFER_SIZE];
                memset(buf, '\0', UDP_BUFFER_SIZE);
                sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                ret = recvfrom(server_udpsocketfd, buf, UDP_BUFFER_SIZE - 1, 0,
                               (sockaddr *)&client_address, &client_addrlength);
                if (ret > 0) {
                    sendto(server_udpsocketfd, buf, UDP_BUFFER_SIZE - 1, 0,
                           (sockaddr *)&client_address, client_addrlength);
                }
            } else if (events[i].events & EPOLLIN) {
                char buf[TCP_BUFFER_SIZE];
                while (1) {
                    memset(buf, '\0', TCP_BUFFER_SIZE);
                    ret = recv(sockfd, buf, TCP_BUFFER_SIZE - 1, 0);
                    if (ret < 0) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            break;
                        }
                        close(sockfd);
                        break;
                    } else if (ret == 0) {
                        close(sockfd);
                    } else {
                        send(sockfd, buf, ret, 0);
                    }
                }
            } else {
                printf("something else happened\n");
            }
        }
    }

    close(server_sockfd);
    return 0;
}