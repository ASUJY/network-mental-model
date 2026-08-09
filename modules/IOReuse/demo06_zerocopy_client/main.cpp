/**
 * 聊天客户端
 * 零拷贝
 */

#define _GNU_SOURCE 1
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 64

int main(int argc, char *argv[]) {

    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    // 创建服务器socket地址
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    // 创建客户端socket文件
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(client_sockfd >= 0);
    // 客户端socket文件连接到服务器socket文件
    if (connect(client_sockfd, (sockaddr *)&server_address,
                sizeof(server_address)) < 0) {
        printf("connection failed\n");
        close(client_sockfd);
        return 1;
    }

    // 创建poll数组，存放需要内核监听的文件描述符
    pollfd fds[2];
    // 注册文件描述符0（标准输入）和文件描述符client_sockfd上的可读事件
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = client_sockfd;
    fds[1].events = POLLIN | POLLHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    // 创建管道
    int ret = pipe(pipefd);
    assert(ret != -1);

    while (1) {
        // 调用IO复用系统调用函数poll，让内核监听指定文件描述符是否有事件发生
        ret = poll(fds, 2, -1);
        if (ret < 0) {
            printf("poll failure\n");
            break;
        }

        if (fds[1].revents & POLLRDHUP) {
            printf("server close the connection\n");
            break;
        } else if (fds[1].revents & POLLIN) {
            memset(read_buf, '\0', BUFFER_SIZE);
            recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
            printf("%s\n", read_buf);
        }

        if (fds[0].revents & POLLIN) {
            // 使用splice将用户输入的数据直接写到client_sockfd上(零拷贝)
            ret = splice(0, NULL, pipefd[1], NULL, 32768,
                         SPLICE_F_MORE | SPLICE_F_MORE);
            ret = splice(pipefd[0], NULL, client_sockfd, NULL, 32768,
                         SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    close(client_sockfd);
    return 0;
}