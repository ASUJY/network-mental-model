//
// Created by ASUJY on 2026/8/1.
//

/*
 * 利用dup函数实现一个基本的CGI服务器
 * */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
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
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // 内核负责监听sock
    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        close(STDOUT_FILENO);
        dup(connfd);
        printf("abcd\n");
        close(connfd);
    }

    close(sock);
    return 0;
}