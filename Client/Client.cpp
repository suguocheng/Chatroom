#include "Client.h"
#include "../ThreadPool.h"

Client::Client(int port) {
    //创建套接字
    connecting_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connecting_sockfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    //创建套接字地址结构
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("192.168.30.170");
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    //连接到服务器
    if (connect(connecting_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to connect to server: " + std::string(strerror(errno)));
    }

    // 添加到 epoll 实例
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // 边缘触发
    event.data.fd = connecting_sockfd;

    //监视事件
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, connecting_sockfd, &event) < 0) {
        throw std::runtime_error("Failed to add socket to epoll");
    }



}

Client::~Client() {
    close(connecting_sockfd);
    close(epfd);
}

void Client::do_read(int conected_sockfd) {

}