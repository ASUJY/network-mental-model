/**
 * 非阻塞connect的实现
 * */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 超时连接函数，参数分别是服务器IP地址、端口号和超时时间(毫秒)
// 函数成功时返回已经处于连接状态的socket，失败则返回-1
int unblock_connect(const char *ip, int port, int time) {
    // 创建服务器socket地址
    int ret = 0;
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    // 创建客户端socket文件
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int fdopt = setnonblocking(client_sockfd);

    // 客户端socket文件连接远程服务器的socket文件
    ret = connect(client_sockfd, (sockaddr *)&server_address,
                  sizeof(server_address));
    if (ret == 0) {
        // 如果连接成功，则恢复sockfd的属性，并立即返回
        printf("connect with server immediatyly\n");
        fcntl(client_sockfd, F_SETFL, fdopt);
        return client_sockfd;
    } else if (errno != EINPROGRESS) {
        // 如果连接没有立即建立，那么只有当errno是EINPROGRESS时才表示连接还在进行
        // 否则出错返回
        printf("unblock connect not support\n");
        return -1;
    }

    // 创建文件描述符集合
    fd_set writefds;
    struct timeval timeout;
    FD_SET(client_sockfd, &writefds);
    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(client_sockfd + 1, NULL, &writefds, NULL, &timeout);
    if (ret <= 0) {
        // select超时或者出错，立即返回
        printf("connection time out\n");
        close(client_sockfd);
        return -1;
    }

    if (!FD_ISSET(client_sockfd, &writefds)) {
        printf("no events on sockfd found\n");
        close(client_sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    // 调用getsockopt来获取并清除client_sockfd上的错误
    if (getsockopt(client_sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        printf("get socket option failed\n");
        close(client_sockfd);
        return -1;
    }

    // 错误号不为0表示连接出错
    if (error != 0) {
        printf("connection failed after select with the error: %d\n", error);
        close(client_sockfd);
        return -1;
    }
    // 连接成功
    printf("connection ready after select with the socket: %d\n",
           client_sockfd);
    fcntl(client_sockfd, F_SETFL, fdopt);
    return client_sockfd;
}

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int client_sockfd = unblock_connect(ip, port, 10);
    if (client_sockfd < 0) {
        return 1;
    }

    close(client_sockfd);

    return 0;
}