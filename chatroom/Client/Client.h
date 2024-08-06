#pragma once

#include "../ThreadPool/ThreadPool.h"
#include "Account_UI.h"
#include <iostream> //输入输出
#include <vector> //数组容器
#include <sys/epoll.h> //epoll
#include <sys/socket.h> //套接字连接
#include <unistd.h> //文件操作
#include <sys/types.h>
#include <arpa/inet.h> //struct sockaddr_in
#include <cstring> //包含memset函数
#include <fcntl.h> //设置非阻塞io
#include <nlohmann/json.hpp> //JSON库

using json = nlohmann::json;

class Client {
public:
    Client(int port);
    ~Client();

    void do_write();
    void do_read();
    
private:
    int connecting_sockfd;
    struct sockaddr_in addr;

    ThreadPool pool;
    // int epfd;
    // std::vector<struct epoll_event> events;

    json j;
};