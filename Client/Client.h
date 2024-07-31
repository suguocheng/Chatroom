#include <iostream> //输入输出
#include <vector> //数组容器
#include <thread> //线程
#include <mutex> //互斥锁
#include <sys/epoll.h> //epoll
#include <sys/socket.h> //套接字连接
#include <unistd.h> //文件操作
#include <arpa/inet.h> //struct sockaddr_in
#include <cstring> //包含memset函数
#include <fcntl.h> //设置非阻塞io

class Client {
public:
    Client(int port);
    ~Client();

    void do_read(int connecting_sockfd);
    
private:
    int connecting_sockfd;
    struct sockaddr_in addr;
    ThreadPool pool;
    int epfd;
    std::vector<struct epoll_event> events;
};