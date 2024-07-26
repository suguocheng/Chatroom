#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <cstdlib>
#include <mutex>

using boost::asio::ip::tcp; //使用命名空间，后面就可以缩写节省代码

class Session : public std::enable_shared_from_this<Session> { //继承了std::enable_shared_from_this模板类，支持对象从其成员函数内部安全地创建指向自身的std::shared_ptr
public:
    Session(tcp::socket socket, std::vector<std::shared_ptr<Session>>& sessions, std::mutex& mutex) //Session的构造函数，传入套接字，会话列表以及互斥锁
        : socket_(std::move(socket)), sessions_(sessions), mutex_(mutex) { //初始化成员变量
        std::cout << "新会话启动\n"; //构造函数里输出会话开始
    }

    void start() {
        do_read();
    }

private:
    void do_read() { //读取函数
        auto self=shared_from_this(); //shared_from_this()获取所在对象的智能指针，之后加入Lambda表达式的捕获列表，防止异步操作中对象被销毁
        socket_.async_read_some(boost::asio::buffer(data_, max_length), //async_read_some异步读取数据到data_中，数据到达后通过回调函数通知，避免阻塞
            [this, self](boost::system::error_code ec, std::size_t length) { //Lambda 表达式，捕获列表里的变量，还有参数变量都是它可以使用的值，这里它作为回调函数。捕获列表里的变量的生命周期会与Lambda表达式一致
                if (!ec) { //ec表示错误状态，返回1即读取成功，返回0即读取失败
                    std::cout << "接收到：" << std::string(data_, length) << "\n"; //输出读取内容
                    broadcast(data_, length); //广播到其他客户端
                    do_read(); //一直循环读取，保证读取的即时性
                } else { //读取失败
                    std::cerr << "读取失败：" << ec.message() << "\n"; //输出读取错误
                }
            });
    }

    void broadcast(const char* message, std::size_t length) {
        std::lock_guard<std::mutex> lock(mutex_); //智能持有锁释放锁
        std::cout << "广播消息：" << std::string(message, length) << "\n"; //输出广播数据
        std::size_t count = 0; //记录要广播的会话数
        for (auto& session : sessions_) { //循环会话列表
            if (session != shared_from_this()) { // 避免广播给发送者
                boost::asio::async_write(session->socket_, boost::asio::buffer(message, length), //async_write_some异步写入数据到message中，数据到达后通过回调函数通知，避免阻塞
                    [this, count](boost::system::error_code ec, std::size_t /*length*/) { ////Lambda 表达式，捕获列表里的变量，还有参数变量都是它可以使用的值，这里它作为回调函数。捕获列表里的变量的生命周期会与Lambda表达式一致
                        if (ec) { //ec表示错误状态，返回1即读取成功，返回0即读取失败
                            std::cerr << "Broadcast error to session " << count << ": " << ec.message() << "\n";
                        } else {
                            std::cout << "成功发送消息到会话 " << count << "\n";
                        }
                    });
                ++count;
            }
        }
        if (count == 0) { //没有要广播的会话
            std::cout << "没有要广播的会话\n";
        }
    }

    tcp::socket socket_; //套接字
    std::vector<std::shared_ptr<Session>>& sessions_; //会话列表
    std::mutex& mutex_; //创建互斥锁
    enum { max_length = 1024 }; //读取数据的最大长度
    char data_[max_length]; //读取的数据
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port) //Server的构造函数。传入io_context以及端口号
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), io_context_(io_context) { //初始化成员变量
        std::cout << "服务器在端口 " << port << " 启动" << "\n"; //输出服务器开始运行
        do_accept(); //构造函数中直接执行do_accept()函数
    }

private:
    void do_accept() { //接受连接请求的函数
        acceptor_.async_accept( //异步接受连接请求
            [this](boost::system::error_code ec, tcp::socket socket) { //Lambda表达式，捕获列表里的变量，还有参数变量都是它可以使用的值，这里它作为回调函数。捕获列表里的变量的生命周期会与Lambda表达式一致，socket是新连接的套接字
                if (!ec) { //ec表示错误状态，返回1即连接成功，返回0即连接失败
                    std::cout << "接受来自客户端的连接\n"; //输出与客户端连接成功
                    auto session = std::make_shared<Session>(std::move(socket), sessions_, mutex_); //创建会话对象
                    { //这里的大括号是为了控制锁的持有范围（不是很懂）
                        std::lock_guard<std::mutex> lock(mutex_); //智能持有锁释放锁
                        sessions_.push_back(session); //将新会话添加到会话列表中
                    }
                    session->start(); //启动会话
                } else { //接受连接请求失败
                    std::cerr << "Accept error: " << ec.message() << "\n"; //输出连接失败
                }
                do_accept();//循环接受连接请求
            });
    }

    tcp::acceptor acceptor_; //接受连接请求的类
    boost::asio::io_context& io_context_; //管理异步操作的类
    std::vector<std::shared_ptr<Session>> sessions_; //会话列表
    std::mutex mutex_; //创建互斥锁
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) { //命令行接受一个参数，端口号
            std::cerr << "Usage: async_tcp_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context; //创建管理异步操作的对象

        Server s(io_context, std::atoi(argv[1])); //创建服务器对象，将参数的字符串类型转换为整型

        std::vector<std::thread> threads; //创建线程容器
        for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) { //std::thread::hardware_concurrency()是最大线程数
            threads.emplace_back([&io_context] { //创建线程对象
                io_context.run(); //启动io_context的事件循环
            });
        }

        for (auto& t : threads) { //等待所有线程完成
            t.join();
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
