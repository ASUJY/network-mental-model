#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
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

    // socket文件和地址绑定在一起
    int ret = bind(server_sockfd, (struct sockaddr *)&server_address,
                   sizeof(server_address));
    assert(ret != -1);

    ret = listen(server_sockfd, 5);
    assert(ret != -1);

    // 创建fd_set集合，存放需要内核监听的文件描述符
    fd_set read_fds;
    fd_set read_fds_tmp;
    FD_ZERO(&read_fds);
    FD_SET(server_sockfd, &read_fds);
    int maxfd = server_sockfd;

    while (1) {
        read_fds_tmp = read_fds;
        // 调用IO复用系统调用函数select，让内核监听指定文件描述符是否有读事件发生
        int ret = select(maxfd + 1, &read_fds_tmp, NULL, NULL, NULL);
        if (ret == -1) {
            perror("select error!");
            exit(-1);
        } else if (ret == 0) {
            continue;
        } else if (ret > 0) {
            // 检测到有文件描述符发生了读事件
            if (FD_ISSET(server_sockfd, &read_fds_tmp)) {
                // server_sockfd发生了读事件，表示有新的客户端连接进来了
                struct sockaddr_in client_addr;
                socklen_t client_addrlength = sizeof(client_addr);
                int client_sockfd =
                    accept(server_sockfd, (sockaddr *)&client_addr,
                           &client_addrlength);

                // 将新的文件描述符加入到监听集合中
                FD_SET(client_sockfd, &read_fds);
                // 更新最大的文件描述符
                maxfd = maxfd > client_sockfd ? maxfd : client_sockfd;
            }

            for (int i = server_sockfd + 1; i <= maxfd; i++) {
                if (FD_ISSET(i, &read_fds_tmp)) {
                    // 说明这个文件描述符发生了读事件，即客户端发来了数据
                    char buf[1024] = {0};
                    int len = recv(i, buf, sizeof(buf), 0);
                    if (len == -1) {
                        perror("recv error!");
                        exit(-1);
                    } else if (len == 0) {
                        printf("client closed...\n");
                        close(i);
                        FD_CLR(i, &read_fds);
                    } else if (len > 0) {
                        printf("recv buf = %s\n", buf);
                        send(i, buf, strlen(buf) + 1, 0);
                    }
                }
            }
        }
    }
    close(server_sockfd);
    return 0;
}