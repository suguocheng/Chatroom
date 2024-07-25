#include <boost/asio.hpp>
#include <iostream>
#include <deque>
#include <thread>

using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& io, const std::string& host, const std::string& port)
        : io_context_(io), socket_(io), resolver_(io) {
        connect(host, port);
    }

    void write(const std::string& message) {
        io_context_.post([this, message]() {
            bool writing_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(message);
            if (!writing_in_progress) {
                do_write();
            }
        });
    }

private:
    void connect(const std::string& host, const std::string& port) {
        auto endpoints = resolver_.resolve(host, port);
        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    std::cout << "Connected to server.\n";
                    do_read();
                } else {
                    std::cerr << "Connect error: " << ec.message() << "\n";
                }
            });
    }

    void do_read() {
        boost::asio::async_read(socket_, boost::asio::buffer(data_, max_length),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout << "Received: " << std::string(data_, length) << "\n";
                    do_read();
                } else {
                    std::cerr << "Read error: " << ec.message() << "\n";
                }
            });
    }

    void do_write() {
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::cout << "Sent: " << write_msgs_.front() << "\n";
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        do_write();
                    }
                } else {
                    std::cerr << "Write error: " << ec.message() << "\n";
                }
            });
    }

    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    tcp::resolver resolver_;
    enum { max_length = 1024 };
    char data_[max_length];
    std::deque<std::string> write_msgs_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: client <host> <port>\n";
            return 1;
        }

        boost::asio::io_context io;

        Client client(io, argv[1], argv[2]);

        std::thread t([&io](){ io.run(); });

        std::string message;
        while (std::getline(std::cin, message)) {
            client.write(message);
        }

        io.stop();
        t.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
