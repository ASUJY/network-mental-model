//
// Created by ASUJY on 2026/7/29.
//

/*
 * 如果监听队列中处于ESTABLISHED状态的连接对应的客户端出现网络异常（比如掉线），或者提前退出，
 * 那么服务器对这个连接执行的accept调用是否成功？
 * */

#include <arpa/inet.h>
#include <assert.h>
#include <iostream>
#include <libgen.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        std::cout << "usage: " << basename(argv[0]) << " ip_address port_number"
                  << std::endl;
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    /* 暂停20秒以等待客户端连接和相关操作(掉线或者退出)完成 */
    sleep(20);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);
    if (connfd < 0) {
        std::cout << "errno is: " << errno << std::endl;
    } else {
        /* 接受连接成功则打印出客户端的IP地址和端口号 */
        char remote[INET_ADDRSTRLEN];
        std::cout << "connected with ip: "
                  << inet_ntop(AF_INET, &client.sin_addr, remote,
                               INET_ADDRSTRLEN)
                  << " and port: " << ntohs(client.sin_port) << std::endl;
        close(connfd);
    }

    close(sock);
    return 0;
}