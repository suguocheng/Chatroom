#pragma once

#include "../ThreadPool/ThreadPool.h"
#include "RedisManager.h"
#include "../log/mars_logger.h"
#include <unordered_map> // 哈希表
#include <iostream> //输入输出
#include <vector> //数组容器
#include <sys/epoll.h> //epoll
#include <sys/socket.h> //套接字连接
#include <unistd.h> //文件操作
#include <arpa/inet.h> //struct sockaddr_in
#include <cstring> //包含memset函数
#include <fcntl.h> //设置非阻塞io
#include <nlohmann/json.hpp> //JSON库

using json = nlohmann::json;

class Server {
public:
    explicit Server(int port);
    ~Server();

    
private:
    void do_recv(int connected_sockfd);
    void do_send(int connected_sockfd, const json& j);
    void heartbeat(int connected_sockfd);

    int listening_sockfd;
    struct sockaddr_in addr;

    // std::vector<int> connected_sockets;

    ThreadPool pool;
    int epfd;
    std::vector<struct epoll_event> events;

    RedisManager redisManager;

    std::unordered_map<std::string, int> map;
};

