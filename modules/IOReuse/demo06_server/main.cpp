
/**
 * 聊天服务器程序
 */

#define _GNU_SOURCE 1

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define USER_LIMIT 5   // 最大用户数量
#define BUFFER_SIZE 64 // 读缓冲区的大小
#define FD_LIMIT 65535 // 文件描述符数量限制

/**
 * 客户数据:
 * 客户端socket地址
 * 待写到客户端的数据的位置
 * 从客户端读入的数据
 */
struct client_data {
    sockaddr_in addr;
    char *write_buf;
    char buf[BUFFER_SIZE];
};

// 将文件描述符设置为非阻塞
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address, port_number\n", basename(argv[0]));
        return 1;
    }

    // 创建服务器socket地址
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    // 创建服务器socket文件
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_sockfd >= 0);

    // 绑定 服务器socket文件和服务器socket地址
    int ret = bind(server_sockfd, (sockaddr *)&server_address,
                   sizeof(server_address));
    assert(ret != -1);

    // 让内核监听 服务器socket(建立TCP连接)
    ret = listen(server_sockfd, 8);
    assert(ret != -1);

    // 创建users数组，分配FD_LIMIT个client_data对象，可以预期：
    // 每个可能的socket连接都可以获得一个这样的对象，并且socket
    // 的值可以直接用来索引（作为数组的下标）socket连接对应的client_data对象，
    // 这是将socket和客户数据关联的简单而高效的方式。
    client_data *users = new client_data[FD_LIMIT];

    // 创建文件描述符集合
    // 尽管我们分配了足够多的client_data对象，但为了提高poll的性能，仍然有必要限制用户的数量
    pollfd fds[USER_LIMIT + 1];
    int user_counter = 0;
    for (int i = 1; i <= USER_LIMIT; ++i) {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    fds[0].fd = server_sockfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while (1) {
        // 调用IO复用系统调用函数poll，让内核监听指定文件描述符是否有事件发生
        ret = poll(fds, user_counter + 1, -1);
        if (ret < 0) {
            printf("poll failure]n");
            break;
        }

        for (int i = 0; i < user_counter + 1; ++i) {
            if ((fds[i].fd == server_sockfd) && fds[i].revents & POLLIN) {
                // server_sockfd发生了读事件，表示有新的客户端连接进来了
                sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(server_sockfd, (sockaddr *)&client_address,
                                    &client_addrlength);
                if (connfd < 0) {
                    printf("errno is: %d\n", errno);
                    continue;
                }
                // 如果请求太多，则关闭新到的连接
                if (user_counter >= USER_LIMIT) {
                    const char *info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }
                // 对于新的连接，同时修改fds和users数组
                // 前文已经提到，users[connfd]对应于新连接文件描述符connfd的客户数据
                user_counter++;
                users[connfd].addr = client_address;
                setnonblocking(connfd);
                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                printf("comes a new user, now have %d users\n", user_counter);
            } else if (fds[i].revents & POLLERR) {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, '\0', 100);
                socklen_t length = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors,
                               &length) < 0) {
                    printf("get socket option failed\n");
                }
                continue;
            } else if (fds[i].revents & POLLRDHUP) {
                // 如果客户端关闭连接，则服务器也关闭对应的连接，并将用户总数减1
                users[fds[i].fd] = users[fds[user_counter].fd];
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                i--;
                user_counter--;
                printf("a client left\n");
            } else if (fds[i].revents & POLLIN) {
                int connfd = fds[i].fd;
                memset(users[connfd].buf, '\0', BUFFER_SIZE);
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
                printf("get %d bytes of client data %s from %d\n", ret,
                       users[connfd].buf, connfd);
                if (ret < 0) {
                    // 如果读操作出错，则关闭连接
                    if (errno != EAGAIN) {
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                } else if (ret == 0) {

                } else {
                    // 如果接收到客户数据，则通知其他socket连接准备写数据
                    for (int j = 1; j <= user_counter; ++j) {
                        if (fds[j].fd == connfd) {
                            continue;
                        }
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            } else if (fds[i].revents & POLLOUT) {
                int connfd = fds[i].fd;
                if (!users[connfd].write_buf) {
                    continue;
                }
                ret = send(connfd, users[connfd].write_buf,
                           strlen(users[connfd].write_buf), 0);
                users[connfd].write_buf = NULL;
                // 写完数据后需要重新注册fds[i]上的可读事件
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete[] users;
    close(server_sockfd);

    return 0;
}