#include "Server.h"
#include "../ThreadPool.h"

Server::Server(int port) {
    //创建套接字
    listening_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_sockfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    //设置非阻塞模式
    int flags = fcntl(listening_sockfd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl(F_GETFL) failed" << std::endl;
        return;
    }
    if (fcntl(listening_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl(F_SETFL) failed" << std::endl;
    }

    //初始化套接字协议地址结构
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    //绑定地址
    if (bind(listening_sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    //监听连接
    if (listen(listening_sockfd, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    //创建epoll
    epfd = epoll_create1(0);
    if (epfd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }

    //注册套接字到epoll（ET 模式），用于描述希望epoll监视的事件
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // 边缘触发模式
    event.data.fd = listening_sockfd;

    //监视事件
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listening_sockfd, &event) < 0) {
        throw std::runtime_error("Failed to add socket to epoll");
    }

    do_accept();
}

Server::~Server() {
    close(listening_sockfd);
    close(epfd);
}

void Server::do_accept() {
    while (true) {
        int num_events = epoll_wait(epfd, events.data(), events.size(), -1);
        if (num_events < 0) {
            std::cerr << "epoll_wait failed" << std::endl;
            continue;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == listening_sockfd) {
                // 新连接请求
                int conected_sockfd = accept(listening_sockfd, nullptr, nullptr);
                if (conected_sockfd < 0) {
                    std::cerr << "Accept failed" << std::endl;
                    continue;
                }

                //设置非阻塞模式
                int flags = fcntl(conected_sockfd, F_GETFL, 0);
                if (flags == -1) {
                    std::cerr << "fcntl(F_GETFL) failed" << std::endl;
                    return;
                }
                if (fcntl(conected_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    std::cerr << "fcntl(F_SETFL) failed" << std::endl;
                }

                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET; // 使用边缘触发
                event.data.fd = conected_sockfd;

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, conected_sockfd, &event) < 0) {
                    std::cerr << "Failed to add client socket to epoll" << std::endl;
                    close(conected_sockfd);
                }
            } else {
                // 处理客户端连接
                int conected_sockfd = events[i].data.fd;
                pool.add_task([this, conected_sockfd] {
                    do_read(conected_sockfd);
                    close(conected_sockfd);
                });
            }
        }
    }
}

void Server::do_read(int conected_sockfd) {
    
}