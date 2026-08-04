//
// Created by ASUJY on 2026/7/29.
//

/*
 * 研究backlog参数对listen系统调用对影响
 * */

#include <iostream>
#include <libgen.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>


static bool stop = false;

static void handle_term(int sig) {
    stop = true;
}

int main(int argc, char* argv[]) {
    signal(SIGTERM, handle_term);

    if (argc <= 3) {
        std::cout << "usage: " << basename(argv[0]) << " ip_address port_number backlog" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    /* 创建一个 IPv4 socket 地址 */
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

     ret = listen(sock, backlog);
     assert(ret != -1);

     /* 循环等待连接，直到有SIGTERM信号将它中断 */
     while(!stop) {
         sleep(1);
     }

     /* 关闭socket */
     close(sock);
     return 0;
}