#include "Session.h"

Session::Session(int socket) : sockfd(socket) {
    //创建epoll
    epfd = epoll_create1(0);
    if (epfd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }

    //注册套接字到epoll（ET 模式），用于描述希望epoll监视的事件
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // 边缘触发模式
    event.data.fd = sockfd;

    //监视事件
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event) < 0) {
        throw std::runtime_error("Failed to add socket to epoll");
    }
    
    
}

Session::~Session() {
    close(epfd);
}

void Session::do_write() {

}

void Session::do_read() {
    while (true) {
        //等待事件并处理
        int num_events = epoll_wait(epfd, events.data(), events.size(), -1);
        if (num_events < 0) {
            std::cerr << "epoll_wait failed" << std::endl;
            continue;
        }

        //遍历事件数组
        for (int i = 0; i < num_events; ++i) {
            pool.add_task([this] {
                do_storage();
                close(sockfd);
            });
        }
    }
    
}

void Session::broadcast() {

}

void Session::do_storage() {

}