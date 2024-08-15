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
#include <sys/stat.h> //查看路径状态
#include <sys/sendfile.h> //发送文件

using json = nlohmann::json;

struct len_name {
	unsigned long len;
	char name[1024];
};

class Server {
public:
    explicit Server(int port);
    ~Server();

    
private:
    void do_recv(int connected_sockfd);
    void do_recv_friend_file(int connected_sockfd, std::string UID, std::string friend_UID);
    void do_recv_group_file(int connected_sockfd, std::string UID, std::string GID);
    void do_send(int connected_sockfd, const json& j);
    void do_send_file(int connected_sockfd, std::string file_name);
    void heartbeat(int connected_sockfd);

    int listening_sockfd;

    struct sockaddr_in addr;

    ThreadPool pool;

    int epfd;
    struct epoll_event event;
    struct epoll_event event2;
    std::vector<struct epoll_event> events;

    RedisManager redisManager;

    std::unordered_map<std::string, int> map;
};

