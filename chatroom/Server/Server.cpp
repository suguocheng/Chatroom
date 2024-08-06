#include "Server.h"
#include "../log/mars_logger.h"


Server::Server(int port) : events(10), pool(20) {
    // redisManager = RedisManager{};

    //创建套接字
    listening_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_sockfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    //设置端口复用
    int optval = 1;
    if (setsockopt(listening_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        close(listening_sockfd);
        exit(EXIT_FAILURE);
    }

    //初始化套接字协议地址结构
    addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    //绑定地址
    if (bind(listening_sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LogFatal("bind失败了 苏国诚快去设置socket reuse 唉我靠 我真是服了 逆天了孩子");
    }

    //监听连接
    if (listen(listening_sockfd, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    //设置套接字为非阻塞模式
    int flags = fcntl(listening_sockfd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl(F_GETFL) failed" << std::endl;
        return;
    }
    if (fcntl(listening_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl(F_SETFL) failed" << std::endl;
    }

    LogInfo("成功启动服务器!");

    //创建epoll
    epfd = epoll_create1(0);
    if (epfd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
    
    //注册套接字到epoll（ET 模式），用于描述希望epoll监视的事件
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // 读事件以及边缘触发模式
    event.data.fd = listening_sockfd;

    //监视事件
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listening_sockfd, &event) < 0) {
        throw std::runtime_error("Failed to add socket to epoll");
    }

    while (1) {
        //等待事件并处理，将事件添加到事件数组中
        int num_events = epoll_wait(epfd, events.data(), events.size(), -1);
        if (num_events < 0) {
            std::cerr << "connect_epoll_wait failed" << std::endl;
            continue;
        }

        //遍历事件数组
        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == listening_sockfd) {
                //接收连接
                socklen_t addrlen = sizeof(addr);
                int connected_sockfd = accept(listening_sockfd, (struct sockaddr *)&addr, &addrlen);
                if (connected_sockfd < 0) {
                    std::cerr << "Accept failed" << std::endl;
                }

                LogInfo("成功连接到客户端!");

                //注册套接字到epoll（ET 模式），用于描述希望epoll监视的事件
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET; // 读事件以及边缘触发模式
                event.data.fd = connected_sockfd;

                //监视事件
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, connected_sockfd, &event) < 0) {
                    throw std::runtime_error("Failed to add socket to epoll");
                }

                // connected_sockets.push_back(connected_sockfd);

            } else if(events[i].events & EPOLLIN) {
                pool.add_task([this, fd = events[i].data.fd] {
                    // LogTrace("something read happened");
                    do_read(fd);
                });

            } else if(events[i].events & EPOLLOUT) {
                
            } else if(events[i].events & EPOLLERR) {

            }
        }
    }
}


Server::~Server() {
    close(listening_sockfd);
    close(epfd);
}


void Server::do_read(int connected_sockfd) {
    json j;

    struct msghdr msg = {0};
    struct iovec iov[1];
    char buf[1024] = {0};

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf) - 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    while (1) {

        ssize_t received_len = recvmsg(connected_sockfd, &msg, 0);
        if (received_len < 0) {
            perror("recvmsg");
            return;
        }

        buf[received_len] = '\0'; // 确保字符串结束符

        if (received_len == 0 || buf[0] == '\0') {
            usleep(50000);
            continue; // 继续等待下一个数据
        }

        // LogInfo("buf: {}", buf);

        try {
            j = json::parse(buf); // 解析 JSON 字符串
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
            return;
        }

        if (j["type"] == "log_in") {
            // LogInfo("username = {}", (j["username"]))
            if (j["password"] != redisManager.get_password(j["username"])) {
                j["result"] = "登录失败";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                j["result"] = "登录成功";
                j["UID"] = redisManager.get_UID(j["username"]);
                id_sockmap[j["UID"]] = connected_sockfd;
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            }

        } else if (j["type"] == "sign_up") {
            if (redisManager.add_user(j["username"], j["password"], j["security_question"], j["security_answer"]) == 0) {
                j["result"] = "注册失败";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                j["result"] = "注册成功";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            }

        } else if (j["type"] == "retrieve_password") {
            std::string security_question = redisManager.get_security_question(j["username"]);
            if (security_question == "") {
                j["security_question"] = "找回失败";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                j["security_question"] = security_question;
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            }
            
        } else if (j["type"] == "retrieve_password_confirm_answer") {
            if (j["security_answer"] != redisManager.get_security_answer(j["username"])) {
                j["password"] = "密保答案错误";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                j["password"] = redisManager.get_password(j["username"]);
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            }

        } else if (j["type"] == "change_usename") {
            if (redisManager.modify_username(j["UID"], j["new_username"]) == 0) {
                j["result"] = "修改失败";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                j["result"] = "修改成功";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            }

        } else if (j["type"] == "change_password") {
            if (j["old_password"] != redisManager.get_password(redisManager.get_username(j["UID"]))) {
                j["result"] = "密码错误";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                if (redisManager.modify_password(j["UID"], j["new_password"]) == 0) {
                    j["result"] = "修改失败";
                    pool.add_task([this, connected_sockfd, j] {
                        do_write(connected_sockfd,j);
                    });

                } else {
                    j["result"] = "修改成功";
                    pool.add_task([this, connected_sockfd, j] {
                        do_write(connected_sockfd,j);
                    });

                }
            }

        } else if (j["type"] == "change_security_question") {
            // LogInfo("security_question = {}", (j["new_security_question"]));
            // LogInfo("security_answer = {}", (j["new_security_answer"]));
            if (j["password"] != redisManager.get_password(redisManager.get_username(j["UID"]))) {
                j["result"] = "密码错误";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                if (redisManager.modify_security_question(j["UID"], j["new_security_question"], j["new_security_answer"]) == 0) {
                    j["result"] = "修改失败";
                    pool.add_task([this, connected_sockfd, j] {
                        do_write(connected_sockfd,j);
                    });

                } else {
                    j["result"] = "修改成功";
                    pool.add_task([this, connected_sockfd, j] {
                        do_write(connected_sockfd,j);
                    });
                    
                }
            }

        } else if (j["type"] == "log_out") {
            if (j["password"] != redisManager.get_password(redisManager.get_username(j["UID"]))) {
                j["result"] = "密码错误";
                pool.add_task([this, connected_sockfd, j] {
                    do_write(connected_sockfd,j);
                });

            } else {
                if (redisManager.delete_user(j["UID"]) == 0) {
                    j["result"] = "注销失败";
                    pool.add_task([this, connected_sockfd, j] {
                        do_write(connected_sockfd,j);
                    });

                } else {
                    j["result"] = "注销成功";
                    pool.add_task([this, connected_sockfd, j] {
                        do_write(connected_sockfd,j);
                    });

                }
            }
        } else if (j["type"] == "get_username") {
            j["username"] = redisManager.get_username(j["UID"]);
            pool.add_task([this, connected_sockfd, j] {
                do_write(connected_sockfd,j);
            });

        } else if (j["type"] == "get_security_question") {
            // LogInfo("UID = {}", (j["UID"]));
            j["security_question"] = redisManager.get_security_question(redisManager.get_username(j["UID"]));
            // LogInfo("security_question = {}", (j["security_question"]));
            pool.add_task([this, connected_sockfd, j] {
                do_write(connected_sockfd,j);
            });
        }
    }




    //测试客户端与服务器连通性
    // char buffer[1024];
    // ssize_t bytes_read = read(connected_sockfd, buffer, sizeof(buffer));
    // if (bytes_read > 0) {
    //     buffer[bytes_read] = '\0'; // 确保字符串结束
    //     std::cout << "Received: " << std::string(buffer, bytes_read) << std::endl;

    //     // 将接收到的消息发送回客户端
    //     std::string response(buffer, bytes_read);
    //     std::cout << "send: " << response << std::endl;
    //     pool.add_task([this, connected_sockfd, response] {
    //         do_write(connected_sockfd, response);
    //     });

    // } else if (bytes_read == 0) {
    //     std::cout << "Client disconnected" << std::endl;
    //     close(connected_sockfd);
    // } else {
    //     std::cerr << "Read error" << std::endl;
    // }

}


void Server::do_storage(int connected_sockfd) {
    //存入redis
}


void Server::do_write(int connected_sockfd, const json& j) {
    std::string json_str = j.dump(); // 将 JSON 对象序列化为字符串

    struct msghdr msg = {0};
    struct iovec iov[1];
    char buf[1024];
    
    // 确保缓冲区大小足够
    if (json_str.size() >= sizeof(buf)) {
        std::cerr << "JSON object too large\n";
        return;
    }
    
    // 准备数据缓冲区
    strncpy(buf, json_str.c_str(), sizeof(buf));
    iov[0].iov_base = buf;
    iov[0].iov_len = json_str.size();
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    ssize_t sent_len = sendmsg(connected_sockfd, &msg, 0);
    if (sent_len < 0) {
        perror("sendmsg");
        return;
    }


    //测试客户端与服务器连通性
    // ssize_t bytes_written = write(connected_sockfd, message.c_str(), message.size());
    // if (bytes_written < 0) {
    //     std::cerr << "Write failed: " << strerror(errno) << std::endl;
    //     // 处理错误，例如关闭连接
    //     close(connected_sockfd);
    // }
}

