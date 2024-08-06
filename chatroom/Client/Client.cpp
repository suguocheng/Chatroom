#include "Client.h"
#include "../log/mars_logger.h"

Client::Client(int port) : pool(2){
    //创建套接字
    connecting_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connecting_sockfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    //创建套接字地址结构
    addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    //连接到服务器
    socklen_t addr_len = sizeof(addr);
    if (connect(connecting_sockfd, (struct sockaddr *)&addr, addr_len) < 0) {
        throw std::runtime_error("Failed to connect to server: " + std::string(strerror(errno)));
    }

    //设置套接字为非阻塞模式，这个要放到连接服务器后面
    // int flags = fcntl(connecting_sockfd, F_GETFL, 0);
    // if (flags == -1) {
    //     std::cerr << "fcntl(F_GETFL) failed" << std::endl;
    //     return;
    // }
    // if (fcntl(connecting_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
    //     std::cerr << "fcntl(F_SETFL) failed" << std::endl;
    // }

    //创建epoll
    // epfd = epoll_create1(0);
    // if (epfd < 0) {
    //     throw std::runtime_error("Failed to create epoll instance");
    // }

    //注册套接字到epoll（ET 模式），用于描述希望epoll监视的事件
    // struct epoll_event event;
    // event.events = EPOLLIN | EPOLLET; // 读写事件以及边缘触发模式
    // event.data.fd = connecting_sockfd;

    //监视事件
    // if (epoll_ctl(epfd, EPOLL_CTL_ADD, connecting_sockfd, &event) < 0) {
    //     throw std::runtime_error("Failed to add socket to epoll");
    // }
    
    // pid_t pid = fork();

    // if (pid < 0) {
    //     // fork() 失败
    //     std::cerr << "fork failed" << std::endl;

    // } else if (pid == 0) {
    //     // 子进程
    //     main_menu_UI(connecting_sockfd);
    // } else {
    //     // 父进程
    //     while (1) {
    //         //等待事件并处理，将事件添加到事件数组中
    //         int num_events = epoll_wait(epfd, events.data(), events.size(), -1);
    //         if (num_events < 0) {
    //             std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
    //             continue;
    //         }

    //         //遍历事件数组
    //         for (int i = 0; i < num_events; ++i) {
    //             if (events[i].events & EPOLLIN) {
    //                 pool.add_task([this] {
    //                     do_read();
    //                 });
    //             } else if (events[i].events & EPOLLOUT) {
    //                 // pool.add_task([this] {
    //                 //     do_write();
    //                 // });
    //             }
    //         }
    //     }
    // }

    // pool.add_task([this] {
    //     main_menu_UI(connecting_sockfd);
    // });

    pool.add_task([this] {
        do_read();
    });

    main_menu_UI(connecting_sockfd);

    // while (1) {
    //     //等待事件并处理，将事件添加到事件数组中
    //     int num_events = epoll_wait(epfd, events.data(), events.size(), -1);
    //     if (num_events < 0) {
    //         // std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
    //         continue;
    //     }


    //     //遍历事件数组
    //     for (int i = 0; i < num_events; ++i) {
    //         if (events[i].events & EPOLLIN) {
    //             pool.add_task([this] {
    //                 do_read();
    //             });
    //         } 
    //         else if (events[i].events & EPOLLOUT) {
    //             // pool.add_task([this] {
    //             //     do_write();
    //             // });
    //         }
    //     }
    // }
}


Client::~Client() {
    close(connecting_sockfd);
}


void Client::do_write() {
    

    //测试客户端与服务器连通性
    // std::string message; //存储写入信息的字符串
    // while(std::getline(std::cin, message)) {
    //     //读取输入
    //     const char* cmessage = message.c_str();
    //     ssize_t bytes_written = write(connecting_sockfd, cmessage, strlen(cmessage));
    //     if (bytes_written < 0) {
    //         std::cerr << "Write failed: " << strerror(errno) << std::endl;
    //     }
    // }
}


void Client::do_read() {
    struct msghdr msg = {0};
    struct iovec iov[1];
    char buf[1024] = {0};

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf) - 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    while(1) {
        ssize_t received_len = recvmsg(connecting_sockfd, &msg, 0);
        if (received_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有数据可读，稍后重试
                continue;
            } else {
                perror("recvmsg");
                return;
            }
        }
        
        buf[received_len] = '\0'; // 确保字符串结束符

        if (received_len == 0 || buf[0] == '\0') {
            usleep(50000);
            continue; // 继续等待下一个数据
        }

        try {
            j = json::parse(buf); // 解析 JSON 字符串
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
            return;
        }

        if (j["type"] == "log_in") {
            std::cout << j["result"] << std::endl;
            if(j["result"] == "登录成功") {
                current_UID = j["UID"];
            } else {
                current_UID = "";
            }
            // LogInfo("current_UID = {}", current_UID);

        } else if (j["type"] == "sign_up") {
            std::cout << j["result"] << std::endl;
            
        } else if (j["type"] == "retrieve_password") {
            std::cout << "你的密保问题：" << j["security_question"] << std::endl;

        } else if (j["type"] == "retrieve_password_confirm_answer") {
            std::cout << "你的密码：" << j["password"] << std::endl;

        } else if (j["type"] == "change_usename") {
            std::cout << j["result"] << std::endl;

        } else if (j["type"] == "change_security_question") {
            std::cout << j["result"] << std::endl;

        } else if (j["type"] == "change_password") {
            std::cout << j["result"] << std::endl;

        } else if (j["type"] == "log_out") {
            std::cout << j["result"] << std::endl;

        } else if (j["type"] == "get_username") {
            std::cout << j["username"] << std::endl;

        } else if (j["type"] == "get_security_question") {
            std::cout << j["security_question"] << std::endl;

        }
    }

    


    //测试客户端与服务器连通性
    // char buffer[1024];
    // ssize_t bytes_read = read(connecting_sockfd, buffer, sizeof(buffer) - 1);
    // if (bytes_read > 0) {
    //     buffer[bytes_read] = '\0'; // 确保字符串结束
    //     std::cout << "Received from server: " << buffer << std::endl;
    // } else if (bytes_read == 0) {
    //     std::cout << "Server disconnected" << std::endl;
    //     close(connecting_sockfd);
    // } else {
    //     std::cerr << "Read error: " << strerror(errno) << std::endl;
    //     close(connecting_sockfd);
    // }
}

