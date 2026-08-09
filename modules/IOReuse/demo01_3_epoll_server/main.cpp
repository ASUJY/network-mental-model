#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

int main() {

    // 创建服务器socket地址
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(6666);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // 创建服务器socket文件
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 服务器socket文件和服务器socket地址进行绑定
    int ret = bind(server_sockfd, (sockaddr *)&server_address,
                   sizeof(server_address));
    assert(ret != -1);

    // 让内核开始监听socket文件
    ret = listen(server_sockfd, 8);
    assert(ret != -1);

    // 在内核中创建一个epoll实例
    int epfd = epoll_create(100);

    // 往epoll实例中添加需要内核监听的文件描述符
    struct epoll_event epev;
    epev.events = EPOLLIN; // 监听读事件
    epev.data.fd = server_sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_sockfd, &epev);

    struct epoll_event epevs[2048];

    while (1) {
        // 调用IO复用系统调用函数epoll，让内核监听指定文件描述符是否有读事件发生
        int ret = epoll_wait(epfd, epevs, 2048, -1);
        if (ret == -1) {
            perror("epoll_wait error!");
            exit(-1);
        }

        // printf("ret = %d\n", ret);
        for (int i = 0; i < ret; i++) {
            int curfd = epevs[i].data.fd;
            if (curfd == server_sockfd) {
                // 检测到server_sockfd发生了读事件，表示有新的客户端连接进来了
                struct sockaddr_in client_addr;
                socklen_t client_addrlength = sizeof(client_addr);
                int client_sockfd =
                    accept(server_sockfd, (sockaddr *)&client_addr,
                           &client_addrlength);

                // 将新的文件描述符加入到监听集合
                // 同时监听这个文件描述符的读事件和写事件
                epev.events = EPOLLIN | EPOLLOUT;
                epev.data.fd = client_sockfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_sockfd, &epev);
            } else {
                if (epevs[i].events & EPOLLOUT) {
                    // 遇到写事件什么都不做
                }
                if (epevs[i].events & EPOLLIN) {
                    // 说明这个文件描述符发生了读事件，即客户端发来了数据
                    char buf[1024] = {0};
                    int len = read(curfd, buf, sizeof(buf));
                    if (len == -1) {
                        perror("recv error!");
                        exit(-1);
                    } else if (len == 0) {
                        printf("client closed...\n");
                        // 从监听集合中删除当前文件描述符(因为客户端关闭了连接，不需要监听了)
                        epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
                        close(curfd);
                    } else if (len > 0) {
                        printf("recv buf = %s\n", buf);
                        write(curfd, buf, strlen(buf) + 1);
                    }
                }
            }
        }
    }
    close(server_sockfd);
    close(epfd);
    return 0;
}