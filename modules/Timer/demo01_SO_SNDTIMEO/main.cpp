#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/**
 * 以connect为例，说明程序中如何使用SO_SNDTIMEO选项来定时
 */

// 超时连接函数
int timeout_connect(const char *ip, int port, int time) {
    // 创建服务器socket地址
    sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    // 创建客户端socket文件
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(client_sockfd >= 0);

    // 通过选项SO_RCVTIMEO 和 SO_SNDTIMEO 所设置的超时时间的类型是timeval，
    // 这和select系统调用的超时参数类型相同
    timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
    // 设置客户端socket文件的SO_SNDTIMEO属性
    int ret = setsockopt(client_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert(ret != -1);

    // 连接服务器
    ret = connect(client_sockfd, (sockaddr *)&server_address,
                  sizeof(server_address));
    if (ret == -1) {
        // 超时对应的错误号EINPROGRESS。下面这个条件成立，我们就可以处理定时任务了
        if (errno == EINPROGRESS) {
            printf("connecting timeout, process timeout logic\n");
            return -1;
        }
        printf("error occur when connecting to server\n");
        return -1;
    }
    return client_sockfd;
}

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int client_sockfd = timeout_connect(ip, port, 10);
    if (client_sockfd < 0) {
        return 1;
    }

    return 0;
}