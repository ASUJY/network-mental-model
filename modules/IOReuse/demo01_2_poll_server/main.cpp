#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {

    // 创建服务器socket地址
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(6666);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // 创建服务器socket文件
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 服务器socket文件和服务器socket地址进行绑定
    int ret = bind(server_sockfd, (struct sockaddr *)&server_address,
                   sizeof(server_address));
    assert(ret != -1);

    // 让内核开始监听socket文件
    ret = listen(server_sockfd, 8);
    assert(ret != -1);

    // 创建poll数组，存放需要内核监听的文件描述符
    struct pollfd fds[2048];
    for (int i = 0; i < 2048; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN; // 监听读事件
    }
    fds[0].fd = server_sockfd;
    int nfds = 0; // 要监听的文件描述符的数量

    while (1) {
        // 调用IO复用系统调用函数poll，让内核监听指定文件描述符是否有读事件发生
        int ret = poll(fds, nfds + 1, -1);
        if (ret == -1) {
            perror("poll error!");
            exit(-1);
        } else if (ret == 0) {
            continue;
        } else if (ret > 0) {
            // 检测到有文件描述符发生了读事件
            if (fds[0].revents & POLLIN) {
                // server_sockfd发生了读事件，表示有新的客户端连接进来了
                struct sockaddr_in client_addr;
                socklen_t client_addrlength = sizeof(client_addr);
                int client_sockfd =
                    accept(server_sockfd, (struct sockaddr *)&client_addr,
                           &client_addrlength);

                // 将新的文件描述符加入到监听集合中
                for (int i = 1; i < 2048; i++) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = client_sockfd;
                        fds[i].events = POLLIN;
                        break;
                    }
                }

                // 更新最大的文件描述符索引
                nfds = nfds > client_sockfd ? nfds : client_sockfd;
            }

            for (int i = 1; i <= nfds; ++i) {
                if (fds[i].revents & POLLIN) {
                    // 说明这个文件描述符发生了读事件，即客户端发来了数据
                    char buf[1024] = {0};
                    int len = recv(fds[i].fd, buf, sizeof(buf), 0);
                    if (len == -1) {
                        perror("recv error!");
                        exit(-1);
                    } else if (len == 0) {
                        printf("client closed...\n");
                        close(fds[i].fd);
                        fds[i].fd = -1;
                    } else if (len > 0) {
                        printf("recv buf = %s\n", buf);
                        send(fds[i].fd, buf, strlen(buf) + 1, 0);
                    }
                }
            }
        }
    }
    close(server_sockfd);
    return 0;
}