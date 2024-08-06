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
#include "../ThreadPool.h"

class Session {
public:
    Session(int socket);
    ~Session();

    void do_write();
    void do_read();
    void broadcast();
    
private:
    void do_storage();

    int sockfd;
    ThreadPool pool;
    int epfd;
    std::vector<struct epoll_event> events;

};