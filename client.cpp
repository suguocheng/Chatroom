#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>

using boost::asio::ip::tcp;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& io_context, const std::string& server, const std::string& port) //Client的构造函数，传入io_context，主机名以及端口号
        : socket_(io_context), resolver_(io_context) { //初始化成员变量
        auto endpoints = resolver_.resolve(server, port); //将主机名和服务名解析为IP地址和端口号
        boost::asio::async_connect(socket_, endpoints, //异步请求连接到服务器
            [this](const boost::system::error_code& ec, const tcp::endpoint&) { //Lambda表达式，捕获列表里的变量，还有参数变量都是它可以使用的值，这里它作为回调函数。捕获列表里的变量的生命周期会与Lambda表达式一致
                if (!ec) { //ec表示错误状态，返回1即连接成功，返回0即连接失败
                    do_read();
                } else {
                    std::cerr << "Connect error: " << ec.message() << "\n";
                }
            });
    }

    tcp::socket socket_; //套接字

private:
    void do_read() { //读取函数
        auto self=shared_from_this(); //shared_from_this()获取所在对象的智能指针，之后加入Lambda表达式的捕获列表，防止异步操作中对象被销毁
        socket_.async_read_some(boost::asio::buffer(buffer_), //async_read_some异步读取数据到data_中，数据到达后通过回调函数通知，避免阻塞
            [this, self](boost::system::error_code ec, std::size_t length) { //Lambda 表达式，捕获列表里的变量，还有参数变量都是它可以使用的值，这里它作为回调函数。捕获列表里的变量的生命周期会与Lambda表达式一致
                if (!ec) { //ec表示错误状态，返回1即读取成功，返回0即读取失败
                    std::cout << "接收到：" << std::string(buffer_, length); //输出读取内容
                    do_read(); //一直循环读取，保证读取的即时性
                } else { //读取失败
                    std::cerr << "Read error: " << ec.message() << "\n"; //输出读取失败
                }
            });
    }

    tcp::resolver resolver_; //解析器
    enum { buffer_size = 1024 }; //读取数据的最大长度
    char buffer_[buffer_size]; //读取的数据
};

int main(int argc, char* argv[]) {
    if (argc != 3) { ////命令行接受两个个参数，主机名和端口号
        std::cerr << "Usage: client <host> <port>\n";
        return 1;
    }

    try {
        boost::asio::io_context io_context; //创建管理异步操作的对象
        auto client = std::make_shared<Client>(io_context, argv[1], argv[2]); //创建客户端对象

        std::thread io_thread([&io_context]() { //创建线程对象
            io_context.run(); //启动io_context的事件循环
        });

        std::string message; //存储写入信息的字符串
        while (std::getline(std::cin, message)) { //读取输入
            boost::asio::write(client->socket_, boost::asio::buffer(message + "\n")); //将信息异步写入套接字
        }

        io_context.stop(); //停止io_context的事件循环
        io_thread.join(); //等待线程结束
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}