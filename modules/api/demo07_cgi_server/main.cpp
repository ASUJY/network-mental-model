//
// Created by ASUJY on 2026/8/1.
//

/*
 * 利用dup函数实现一个基本的CGI服务器
 * 把标准输出重定向到一个网络连接，这样一来，服务器输出到
 * 标准输出到内容就会直接发送到与客户连接对应的socket上。
 * */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    // 创建socket地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // socket和ip地址进行绑定
    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    // 内核负责监听sock
    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        // 先关闭标准输出文件描述符
        close(STDOUT_FILENO);
        // 复制socket文件描述符，因为dup总是返回系统中最小的可用文件描述符，
        // 所以它的返回值实际是1（即标准输出文件描述符）
        dup(connfd);
        printf("abcd\n");
        close(connfd);
    }

    close(sock);
    return 0;
}