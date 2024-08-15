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
        LogFatal("bind失败了 快去设置socket reuse");
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

    //日志输出启动服务器
    LogInfo("成功启动服务器!");

    //创建epoll
    epfd = epoll_create1(0);
    if (epfd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
    
    //注册套接字到epoll（ET 模式），用于描述希望epoll监视的事件
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
            std::cerr << "epoll_wait failed" << std::endl;
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

                //日志输出连接到客户端
                LogInfo("成功连接到客户端!");
                // LogInfo("connected_socked = {}", connected_sockfd);

                //设置套接字为非阻塞模式
                int flags = fcntl(connected_sockfd, F_GETFL, 0);
                if (flags == -1) {
                    std::cerr << "fcntl(F_GETFL) failed" << std::endl;
                    return;
                }
                if (fcntl(connected_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    std::cerr << "fcntl(F_SETFL) failed" << std::endl;
                }

                //注册套接字到epoll（ET 模式），用于描述希望epoll监视的事件
                event2.events = EPOLLIN | EPOLLET; // 读事件以及边缘触发模式
                event2.data.fd = connected_sockfd;

                //监视事件
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, connected_sockfd, &event2) < 0) {
                    throw std::runtime_error("Failed to add socket to epoll");
                }

                // connected_sockets.push_back(connected_sockfd);

            } else if (events[i].events & EPOLLIN) {

                //如果监控到读事件就将接收函数添加到线程池运行
                pool.add_task([this, fd = events[i].data.fd] {
                    // LogTrace("something read happened");
                    do_recv(fd);
                });
                
            } else if (events[i].events & EPOLLOUT) {
                
            } else if (events[i].events & EPOLLERR) {

            }
        }
    }
}


Server::~Server() {
    close(listening_sockfd);
    close(epfd);
}


void Server::do_recv(int connected_sockfd) {
    //创建json
    json j;

    //存储接收信息的结构体
    struct msghdr msg = {0};

    //描述数据缓冲区的结构体
    struct iovec iov[1];

    //设置缓冲区
    char buf[1024] = {0};
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf) - 1;

    //只使用一个缓冲区
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    while (1) {
    
        //接收数据
        ssize_t received_len = recvmsg(connected_sockfd, &msg, 0);
        if (received_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 当前没有数据可读，稍后重试
                return;
            } else if (errno == EINTR) {
                // 被信号中断，重新尝试
                return;
            } else {
                // 处理其他错误
                perror("recvmsg");
                return;
            }

        //客户端断开
        } else if (received_len == 0) {
            LogInfo("客户端断开连接");
            return;
        }

        buf[received_len] = '\0'; // 确保字符串结束符

        // LogInfo("buf: {}", buf);

        //解析JSON字符串
        try {
            j = json::parse(buf);
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
            return;
        }

        //接收登录类型数据
        if (j["type"] == "log_in") {
            if (redisManager.get_UID(j["username"]) == "") {
                //用户不存在
                j["result"] = "用户不存在";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                //与redis比较密码
                if (j["password"] != redisManager.get_password(j["username"])) {
                    
                    //密码错误
                    j["result"] = "密码错误";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {
                    //查询用户是否在线
                    auto it = map.find(redisManager.get_UID(j["username"]));

                    //在线
                    if (it != map.end()) {
                        j["result"] = "用户已经登录";
                        
                        //将数据发送回原客户端
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });

                    //不在线
                    } else {
                        //登录成功
                        j["result"] = "登录成功";
                        j["UID"] = redisManager.get_UID(j["username"]);

                        //将uid与在线套接字绑定，以确定在线状态
                        map[j["UID"]] = connected_sockfd;
                        
                        //只要登录就发送粗略的消息通知(检查通知是否存在的方式有问题)
                        json j2;
                        std::vector<std::string> notifications;
                        j2["type"] = "notice";
                        
                        redisManager.get_notification(j["UID"], "friend_request", notifications);
                        if (notifications.empty()) {
                            j2["friend_request_notification"] = 0;
                        } else {
                            j2["friend_request_notification"] = 1;
                        }

                        notifications.clear();
                        redisManager.get_notification(j["UID"], "group_request", notifications);
                        if (notifications.empty()) {
                            j2["group_request_notification"] = 0;
                        } else {
                            j2["group_request_notification"] = 1;
                        }

                        notifications.clear();
                        redisManager.get_notification(j["UID"], "message", notifications);
                        if (notifications.empty()) {
                            j2["message_notification"] = 0;
                        } else {
                            j2["message_notification"] = 1;
                        }

                        notifications.clear();
                        redisManager.get_notification(j["UID"], "file", notifications);
                        if (notifications.empty()) {
                            j2["file_notification"] = 0;
                        } else {
                            j2["file_notification"] = 1;
                        }

                        // LogInfo("message_notification = {}", (j2["message_notification"]));
                        
                        pool.add_task([this, connected_sockfd, j2] {
                            do_send(connected_sockfd,j2);
                        });

                        usleep(10000);
                        
                        //将数据发送回原客户端
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });
                    }
                }
            }
            
        //接收注册类型数据
        } else if (j["type"] == "sign_up") {

            //在redis中添加用户
            if (redisManager.add_user(j["username"], j["password"], j["security_question"], j["security_answer"]) == 0) {
                
                //注册失败
                j["result"] = "注册失败";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                //注册成功
                j["result"] = "注册成功";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }
        
        //接收找回密码类型数据
        } else if (j["type"] == "retrieve_password") {
            
            //创建字符串临时存储密保问题
            std::string security_question = redisManager.get_security_question(j["username"]);

            //redis中密保问题获取失败
            if (security_question == "") {

                //找回失败
                j["security_question"] = "找回失败";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                
                //找回成功
                j["security_question"] = security_question;

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }
        
        //接收找回密码确认答案类型数据
        } else if (j["type"] == "retrieve_password_confirm_answer") {

            //与redis中密保答案比较
            if (j["security_answer"] != redisManager.get_security_answer(j["username"])) {

                //密保答案错误
                j["password"] = "密保答案错误";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });
            
            } else {

                //密保答案正确发送密码
                j["password"] = redisManager.get_password(j["username"]);

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }

        //接收更改用户名类型数据
        } else if (j["type"] == "change_usename") {

            //redis修改用户名
            if (redisManager.modify_username(j["UID"], j["new_username"]) == 0) {

                //修改失败
                j["result"] = "修改失败";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                //修改成功
                j["result"] = "修改成功";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }

        //接收修改密码类型数据
        } else if (j["type"] == "change_password") {

            //与redis中旧密码比较
            if (j["old_password"] != redisManager.get_password(redisManager.get_username(j["UID"]))) {

                //密码错误
                j["result"] = "密码错误";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                //redis修改密码
                if (redisManager.modify_password(j["UID"], j["new_password"]) == 0) {

                    //修改失败
                    j["result"] = "修改失败";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {

                    //修改成功
                    j["result"] = "修改成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                }
            }

        //接收更改密保问题类型数据
        } else if (j["type"] == "change_security_question") {
            
            //与redis中密码比较
            if (j["password"] != redisManager.get_password(redisManager.get_username(j["UID"]))) {

                //密码错误
                j["result"] = "密码错误";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                
                //在redis中修改密保问题及答案
                if (redisManager.modify_security_question(j["UID"], j["new_security_question"], j["new_security_answer"]) == 0) {

                    //修改失败
                    j["result"] = "修改失败";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {

                    //修改成功
                    j["result"] = "修改成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                    
                }
            }
        
        //接收到退出登录类型数据
        } else if (j["type"] == "quit_log") {

            //解除uid与在线套接字的绑定
            map.erase(j["UID"]);

            j["result"] = "退出成功";

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

        //接收到注销帐号类型数据
        } else if (j["type"] == "log_out") {

            //与redis中密码比较
            if (j["password"] != redisManager.get_password(redisManager.get_username(j["UID"]))) {

                //密码错误
                j["result"] = "密码错误";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                //redis中删除用户
                if (redisManager.delete_user(j["UID"]) == 0) {

                    //注销失败
                    j["result"] = "注销失败";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {

                    //注销成功
                    j["result"] = "注销成功";

                    //解除uid与在线套接字的绑定
                    map.erase(j["UID"]);

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                }
            }
        
        //接收到获取用户名类型信息
        } else if (j["type"] == "get_username") {

            //创建字符串临时存储用户名
            std::string username = redisManager.get_username(j["UID"]);
            
            //redis中获取用户名失败
            if (username == "") {

                //获取失败
                j["username"] = "获取失败";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                
                //获取成功
                j["username"] = username;

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });
            }
            
        //接收到获取密保问题类型数据
        } else if (j["type"] == "get_security_question") {
            
            //创建字符串临时存储密保问题
            std::string security_question = redisManager.get_security_question(redisManager.get_username(j["UID"]));

            //获取密保问题失败
            if (security_question == "") {

                //获取失败
                j["security_question"] = "获取失败";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                //获取成功
                j["security_question"] = security_question;

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });
            }
            
        //获取到添加好友类型数据
        } else if (j["type"] == "add_friend") {

            //获取用户名失败
            if (redisManager.get_username(j["search_UID"]) == "") {

                //用户不存在
                j["result"] = "该用户不存在";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                if (j["UID"] == j["search_UID"]) {

                    //申请失败
                    j["result"] = "不能加自己为好友";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                } else {
                    //存储好友申请通知到redis
                    std::string UID = j["UID"].get<std::string>();
                    std::string notification = "用户" + UID + "想添加您为好友";

                    //存储好友申请通知失败
                    if (redisManager.add_notification(j["search_UID"], "friend_request", notification) == 0) {

                        //申请失败
                        j["result"] = "申请失败";

                        //将数据发送回原客户端
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });

                    } else {

                        //申请成功
                        j["result"] = "申请成功";

                        //查询用户是否在线
                        auto it = map.find(j["search_UID"]);

                        //在线
                        if (it != map.end()) {

                            json j2;
                            j2["type"] = "notice";
                            j2["friend_request_notification"] = 1;

                            int connected_sockfd2 = it->second;

                            pool.add_task([this, connected_sockfd2, j2] {
                                do_send(connected_sockfd2, j2);
                            });
                        
                        }

                        //将数据发送回原客户端
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });

                    }
                }
            }

        } else if (j["type"] == "view_friend_requests") {
            std::vector<std::string> notifications;
            redisManager.get_notification(j["UID"], "friend_request", notifications);
            j["friend_requests"] = notifications;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

        } else if (j["type"] == "agree_to_friend_request") {

            // LogInfo("准备添加好友");

            if (redisManager.get_username(j["request_UID"]) == "") {

                //用户不存在
                j["result"] = "请输入正确用户";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                if (redisManager.add_friend(j["UID"], j["request_UID"]) == 0) {
                    j["result"] = "添加好友失败";
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {
                    std::string request_UID = j["request_UID"].get<std::string>();
                    std::string notification = "用户" + request_UID + "想添加您为好友";

                    // LogInfo("准备删除通知");
                    redisManager.delete_notification(j["UID"], "friend_request", notification);

                    j["result"] = "添加好友成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                }
            }
            
        } else if (j["type"] == "view_friends_list") {
            std::vector<std::string> friends_UID, friends_name;
            
            //获取好友id
            // LogInfo("获取好友uid列表");
            redisManager.get_friends(j["UID"], friends_UID);
            // LogInfo("获取好友uid列表完成");
            //获取好友用户名
            // LogInfo("获取好友用户名列表");
            for(int i = 0; i < friends_UID.size() ; ++i) {
                friends_name.push_back(redisManager.get_username(friends_UID[i]));
            }
            // LogInfo("获取好友用户名列表完成");
            j["friends_UID"] = friends_UID;
            j["friends_name"] = friends_name;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

        } else if (j["type"] == "confirmed_as_friend") {
            std::vector<std::string> friends_UID;
            redisManager.get_friends(j["UID"], friends_UID);

            j["result"] = "该用户不是好友";
            for(int i = 0; i < friends_UID.size() ; ++i) {
                if (friends_UID[i] == j["friend_UID"]) {
                    j["result"] = "该用户是好友";
                    break;
                }
            }

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "check_online_status") {
            //查询用户是否在线
            auto it = map.find(j["UID"]);

            //在线
            if (it != map.end()) {
                j["result"] = "在线";
            } else {
                j["result"] = "不在线";
            }

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "delete_friend") {
            if (redisManager.delete_friend(j["UID"], j["friend_UID"]) == 0) {
                j["result"] = "删除失败";
            } else {
                j["result"] = "删除成功";
            }

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

        } else if (j["type"] == "block_friend") {

            //没被屏蔽
            if (redisManager.check_block_friend(j["UID"], j["friend_UID"]) == 0) {

                //屏蔽失败
                if (redisManager.add_block_friend(j["UID"], j["friend_UID"]) == 0) {
                    j["result"] = "屏蔽失败";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                } else {
                    j["result"] = "屏蔽成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                }

            //已被屏蔽
            } else {
                j["result"] = "该好友已被屏蔽";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });
            }

        } else if (j["type"] == "unblock_friend") {

            //已被屏蔽
            if (redisManager.check_block_friend(j["UID"], j["friend_UID"]) != 0) {

                //解除屏蔽失败
                if (redisManager.delete_block_friend(j["UID"], j["friend_UID"]) == 0) {
                    j["result"] = "解除屏蔽失败";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                } else {
                    j["result"] = "解除屏蔽成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                }
            
            //没被屏蔽
            } else {
                j["result"] = "该好友未被屏蔽";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });
            }
        } else if (j["type"] == "get_chat_messages") {

            std::vector<std::string> messages;
            redisManager.get_private_chat_messages(j["UID"], j["friend_UID"], messages);

            j["messages"] = messages;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

        } else if (j["type"] == "send_chat_messages") {
            //已被屏蔽
            if (redisManager.check_block_friend(j["friend_UID"], j["UID"]) != 0) {
                j["result"] = "该好友已将您屏蔽";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                
                //发送失败
                if (redisManager.add_private_chat_message(j["UID"], j["friend_UID"], j["message"]) != 1) {
                    j["result"] = "发送失败";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                } else {

                    //存储消息通知到redis
                    std::string UID = j["UID"].get<std::string>();
                    std::string notification = "好友" + redisManager.get_username(UID) + "(UID为:" + UID + ")" + "给你发来了新消息";

                    std::vector<std::string> notifications;
                    redisManager.get_notification(j["friend_UID"], "message", notifications);

                    //如果重复就删除以前的消息通知保留最新的
                    for (const auto& n : notifications) {
                        if (notification == n) {
                            redisManager.delete_notification(j["friend_UID"], "message", n);
                        }
                    }

                    //存储消息通知失败
                    if (redisManager.add_notification(j["friend_UID"], "message", notification) == 0) {

                        //存储失败
                        j["result"] = "发送失败";

                        //将数据发送回原客户端
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });

                    } else {

                        //存储成功
                        j["result"] = "发送成功";

                        //查询用户是否在线
                        auto it = map.find(j["friend_UID"]);

                        //在线
                        if (it != map.end()) {

                            json j2;
                            j2["type"] = "notice";
                            j2["message_notification"] = 1;

                            int connected_sockfd2 = it->second;

                            pool.add_task([this, connected_sockfd2, j2] {
                                do_send(connected_sockfd2, j2);
                            });
                        
                        }

                        //将数据发送回原客户端
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });
                    }
                }
            }
        } else if (j["type"] == "view_messages") {
            std::vector<std::string> notifications;
            redisManager.get_notification(j["UID"], "message", notifications);
            j["messages"] = notifications;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "handle_new_friend_messages") {
            std::string friend_UID = j["friend_UID"].get<std::string>();
            std::string notification = "好友" + redisManager.get_username(friend_UID) + "(UID为:" + friend_UID + ")" + "给你发来了新消息";

            redisManager.delete_notification(j["UID"], "message", notification);

        } else if (j["type"] == "create_group") {
            if(redisManager.add_group(j["group_name"], j["group_owner_UID"]) == 0) {

                //创建失败
                j["result"] = "创建失败";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                
                //创建成功
                j["result"] = "创建成功";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }
        } else if (j["type"] == "add_group") {
            //获取群组名失败
            if (redisManager.get_group_name(j["search_GID"]) == "") {

                //群组不存在
                j["result"] = "该群组不存在";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                int n = 0;
                std::vector<std::string> groups_GID;
                redisManager.get_groups(j["UID"], groups_GID);

                for(int i = 0; i < groups_GID.size() ; ++i) {
                    if (groups_GID[i] == j["search_GID"]) {
                        n = 1;
                        break;
                    }
                }

                if (n == 1) {
                    j["result"] = "您已在该群组中";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                } else {
                    //存储好友申请通知到redis
                    std::string UID = j["UID"].get<std::string>();
                    std::string search_GID = j["search_GID"].get<std::string>();
                    std::string notification = "用户" + UID + "想加入群聊" + search_GID;

                    //存储群组申请通知
                    redisManager.add_notification(redisManager.get_group_owner_UID(j["search_GID"]), "group_request", notification);

                    std::vector<std::string> administrators_UID;
                    redisManager.get_administrators(j["search_GID"], administrators_UID);

                    for (const auto& administrator_UID : administrators_UID) {
                        redisManager.add_notification(administrator_UID, "group_request", notification);
                    }

                    //申请成功
                    j["result"] = "申请成功";

                    //查询群主是否在线
                    auto it = map.find(redisManager.get_group_owner_UID(j["search_GID"]));

                    //在线
                    if (it != map.end()) {

                        json j2;
                        j2["type"] = "notice";
                        j2["group_request_notification"] = 1;

                        int connected_sockfd2 = it->second;

                        pool.add_task([this, connected_sockfd2, j2] {
                            do_send(connected_sockfd2, j2);
                        });
                    
                    }

                    for (const auto& administrator_UID : administrators_UID) {
                        //查询管理员是否在线
                        auto it = map.find(administrator_UID);

                        //在线
                        if (it != map.end()) {

                            json j3;
                            j3["type"] = "notice";
                            j3["group_request_notification"] = 1;

                            int connected_sockfd3 = it->second;

                            pool.add_task([this, connected_sockfd3, j3] {
                                do_send(connected_sockfd3, j3);
                            });
                        
                        }
                    }

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                }
            }
            
        } else if (j["type"] == "view_group_requests") {
            std::vector<std::string> notifications;
            redisManager.get_notification(j["UID"], "group_request", notifications);
            j["group_requests"] = notifications;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "agree_to_group_request") {

            if (redisManager.get_username(j["request_UID"]) == "") {

                //用户不存在
                j["result"] = "请输入正确用户";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                if (redisManager.get_group_name(j["request_GID"]) == "") {
                    j["result"] = "请输入正确群组";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {
                    if (redisManager.add_group_member(j["request_GID"], j["request_UID"]) == 0) {
                        j["result"] = "加入群组失败";
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });

                    } else {
                        std::string request_UID = j["request_UID"].get<std::string>();
                        std::string request_GID = j["request_GID"].get<std::string>();
                        std::string notification = "用户" + request_UID + "想加入群聊" + request_GID;

                        // LogInfo("准备删除通知");
                        //删除群组申请通知
                        redisManager.delete_notification(redisManager.get_group_owner_UID(j["request_GID"]), "group_request", notification);

                        std::vector<std::string> administrators_UID;
                        redisManager.get_administrators(j["request_GID"], administrators_UID);

                        for (const auto& administrator_UID : administrators_UID) {
                            redisManager.delete_notification(administrator_UID, "group_request", notification);
                        }

                        j["result"] = "加入群组成功";

                        //将数据发送回原客户端
                        pool.add_task([this, connected_sockfd, j] {
                            do_send(connected_sockfd,j);
                        });

                    }
                }
            }
            
        } else if (j["type"] == "view_groups_list") {
            std::vector<std::string> groups_GID, groups_name;

            redisManager.get_groups(j["UID"], groups_GID);
            
            for(int i = 0; i < groups_GID.size() ; ++i) {
                groups_name.push_back(redisManager.get_group_name(groups_GID[i]));
            }
            
            j["groups_GID"] = groups_GID;
            j["groups_name"] = groups_name;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "confirm_in_group") {
            std::vector<std::string> groups_UID;
            redisManager.get_groups(j["UID"], groups_UID);

            j["result"] = "您不在该群组中";
            for(int i = 0; i < groups_UID.size() ; ++i) {
                if (groups_UID[i] == j["GID"]) {
                    j["result"] = "您在该群组中";
                    break;
                }
            }

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

        } else if (j["type"] == "get_group_name") {
            j["result"] = redisManager.get_group_name(j["GID"]);

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "view_member_list") {
            std::string group_owner_UID, group_owner_username;
            std::vector<std::string> administrators_UID, members_UID, administrators_username, members_username;

            group_owner_UID = redisManager.get_group_owner_UID(j["GID"]);
            group_owner_username = redisManager.get_username(redisManager.get_group_owner_UID(j["GID"]));

            redisManager.get_administrators(j["GID"], administrators_UID);
            redisManager.get_group_members(j["GID"], members_UID);

            for (int i = 0; i < administrators_UID.size(); ++i) {
                administrators_username.push_back(redisManager.get_username(administrators_UID[i]));
            }

            for (int i = 0; i < members_UID.size(); ++i) {
                members_username.push_back(redisManager.get_username(members_UID[i]));
            }

            j["group_owner_UID"] = group_owner_UID;
            j["group_owner_username"] = group_owner_username;
            j["members_UID"] = members_UID;
            j["administrators_UID"] = administrators_UID;
            j["members_username"] = members_username;
            j["administrators_username"] = administrators_username;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "exit_group") {
            if (j["UID"] == redisManager.get_group_owner_UID(j["GID"])) {
                j["result"] = "您是群主，请转让群主后退出群组，或者解散群组";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                if (redisManager.check_administrator(j["GID"], j["UID"]) == 0) {
                    redisManager.delete_group_member(j["GID"], j["UID"]);

                    j["result"] = "退出成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {
                    redisManager.delete_administrator(j["GID"], j["UID"]);
                    redisManager.delete_group_member(j["GID"], j["UID"]);

                    j["result"] = "退出成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });
                }
            }
            
        } else if (j["type"] == "dismiss_group") {
            if (j["UID"] == redisManager.get_group_owner_UID(j["GID"])) {
                redisManager.delete_group(j["GID"]);
                redisManager.delete_group_member(j["GID"], j["UID"]);
                
                std::vector<std::string> administrators_UID, members_UID;
                redisManager.get_administrators(j["GID"], administrators_UID);
                redisManager.get_group_members(j["GID"], members_UID);

                for (int i = 0; i < administrators_UID.size(); ++i) {
                    redisManager.delete_administrator(j["GID"], administrators_UID[i]);
                    redisManager.delete_group_member(j["GID"], administrators_UID[i]);
                }

                for (int i = 0; i < members_UID.size(); ++i) {
                    redisManager.delete_group_member(j["GID"], administrators_UID[i]);
                }

                j["result"] = "解散成功";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                j["result"] = "您不是群主，没有权限解散群组";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });
            }
        } else if (j["type"] == "remove_group_member") {
            if (j["UID"] == redisManager.get_group_owner_UID(j["GID"])) {
                if (redisManager.check_administrator(j["GID"], j["member_UID"]) == 1) {

                    redisManager.delete_administrator(j["GID"], j["member_UID"]);
                    redisManager.delete_group_member(j["GID"], j["member_UID"]);

                    j["result"] = "移除成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {
                    redisManager.delete_group_member(j["GID"], j["member_UID"]);

                    j["result"] = "移除成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                }
            } else if (redisManager.check_administrator(j["GID"], j["UID"]) == 1) {
                if (j["member_UID"] == redisManager.get_group_owner_UID(j["GID"]) || redisManager.check_administrator(j["GID"], j["member_UID"])) {
                    j["result"] = "您没有权限移除成员";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {
                    redisManager.delete_group_member(j["GID"], j["member_UID"]);

                    j["result"] = "移除成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                }
            } else {
                j["result"] = "您没有权限移除成员";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }
        } else if (j["type"] == "set_up_administrator") {
            if (j["UID"] == redisManager.get_group_owner_UID(j["GID"])) {
                redisManager.add_administrator(j["GID"], j["member_UID"]);

                j["result"] = "设置成功";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                j["result"] = "您没有权限设置管理员";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }
        } else if (j["type"] == "remove_administrator") {
            if (j["UID"] == redisManager.get_group_owner_UID(j["GID"])) {
                if (redisManager.check_administrator(j["GID"], j["member_UID"]) == 1) {

                    redisManager.delete_administrator(j["GID"], j["member_UID"]);
                    redisManager.add_group_member(j["GID"], j["member_UID"]);

                    j["result"] = "移除管理员成功";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                } else {
                    j["result"] = "该成员不是管理员";

                    //将数据发送回原客户端
                    pool.add_task([this, connected_sockfd, j] {
                        do_send(connected_sockfd,j);
                    });

                }
                
            } else {
                j["result"] = "您没有权限移除管理员";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }
            
        } else if (j["type"] == "get_group_chat_messages") {
            std::vector<std::string> messages;
            redisManager.get_group_chat_messages(j["GID"], messages);

            j["messages"] = messages;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });
            
        } else if (j["type"] == "send_group_chat_messages") {
            //发送失败
            if (redisManager.add_group_chat_message(j["GID"], j["UID"], j["message"]) != 1) {
                j["result"] = "发送失败";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {

                //存储消息通知到redis
                std::string GID = j["GID"].get<std::string>();
                std::string notification = "群聊" + redisManager.get_group_name(GID) + "(GID为:" + GID + ")" + "有新消息";

                if (redisManager.get_group_owner_UID(GID) != j["UID"]) {
                    std::vector<std::string> notifications;
                    redisManager.get_notification(redisManager.get_group_owner_UID(GID), "message", notifications);

                    //如果重复就删除以前的消息通知保留最新的
                    for (const auto& n : notifications) {
                        if (notification == n) {
                            redisManager.delete_notification(redisManager.get_group_owner_UID(GID), "message", n);
                        }
                    }

                    redisManager.add_notification(redisManager.get_group_owner_UID(GID), "message", notification);

                    //查询用户是否在线
                    auto it = map.find(redisManager.get_group_owner_UID(GID));

                    //在线
                    if (it != map.end()) {

                        json j2;
                        j2["type"] = "notice";
                        j2["message_notification"] = 1;

                        int connected_sockfd2 = it->second;

                        pool.add_task([this, connected_sockfd2, j2] {
                            do_send(connected_sockfd2, j2);
                        });
                    
                    }
                }

                std::vector<std::string> administrators_UID, members_UID;
                redisManager.get_administrators(GID, administrators_UID);
                redisManager.get_group_members(GID, members_UID);

                for (int i = 0; i < administrators_UID.size(); ++i) {

                    if (j["UID"] != administrators_UID[i]) {
                        std::vector<std::string> notifications;
                        redisManager.get_notification(administrators_UID[i], "message", notifications);

                        //如果重复就删除以前的消息通知保留最新的
                        for (const auto& n : notifications) {
                            if (notification == n) {
                                redisManager.delete_notification(administrators_UID[i], "message", n);
                            }
                        }

                        redisManager.add_notification(administrators_UID[i], "message", notification);

                        //查询用户是否在线
                        auto it = map.find(administrators_UID[i]);

                        //在线
                        if (it != map.end()) {

                            json j2;
                            j2["type"] = "notice";
                            j2["message_notification"] = 1;

                            int connected_sockfd2 = it->second;

                            pool.add_task([this, connected_sockfd2, j2] {
                                do_send(connected_sockfd2, j2);
                            });
                        
                        }
                    }

                }

                for (int i = 0; i < members_UID.size(); ++i) {

                    if (j["UID"] != members_UID[i]) {
                        std::vector<std::string> notifications;
                        redisManager.get_notification(members_UID[i], "message", notifications);

                        //如果重复就删除以前的消息通知保留最新的
                        for (const auto& n : notifications) {
                            if (notification == n) {
                                redisManager.delete_notification(members_UID[i], "message", n);
                            }
                        }

                        redisManager.add_notification(members_UID[i], "message", notification);

                        //查询用户是否在线
                        auto it = map.find(members_UID[i]);

                        //在线
                        if (it != map.end()) {

                            json j2;
                            j2["type"] = "notice";
                            j2["message_notification"] = 1;

                            int connected_sockfd2 = it->second;

                            pool.add_task([this, connected_sockfd2, j2] {
                                do_send(connected_sockfd2, j2);
                            });
                        
                        }
                    }

                }

                //存储成功
                j["result"] = "发送成功";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd, j);
                });
            }
        } else if (j["type"] == "handle_new_group_messages") {
            std::string GID = j["GID"].get<std::string>();
            std::string notification = "群聊" + redisManager.get_group_name(GID) + "(GID为:" + GID + ")" + "有新消息";

            redisManager.delete_notification(j["UID"], "message", notification);
            
        } else if (j["type"] == "confirmed_as_block_friend") {
            //已被屏蔽
            if (redisManager.check_block_friend(j["friend_UID"], j["UID"]) != 0) {
                j["result"] = "该好友已将您屏蔽";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            } else {
                j["result"] = "该好友未将您屏蔽";

                //将数据发送回原客户端
                pool.add_task([this, connected_sockfd, j] {
                    do_send(connected_sockfd,j);
                });

            }
            
        } else if (j["type"] == "send_friend_file") {

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd, j);
            });
            
            //设置为阻塞模式
            int flags = fcntl(connected_sockfd, F_GETFL, 0);
            if (flags < 0) {
                std::cerr << "fcntl(F_GETFL) failed: " << strerror(errno) << std::endl;
                return;
            }
            flags &= ~O_NONBLOCK;
            if (fcntl(connected_sockfd, F_SETFL, flags) < 0) {
                std::cerr << "fcntl(F_SETFL) failed: " << strerror(errno) << std::endl;
                return;
            }

            //取消监视
            epoll_ctl(epfd, EPOLL_CTL_DEL, connected_sockfd, NULL);

            pool.add_task([this, fd = connected_sockfd, UID = j["UID"], friend_UID = j["friend_UID"]] {
                do_recv_friend_file(fd, UID, friend_UID);
            });

            //直接返回，要读取文件了，不再读取json数据
            return;
            
        } else if (j["type"] == "view_file") {
            std::vector<std::string> notifications;
            redisManager.get_notification(j["UID"], "file", notifications);
            j["files"] = notifications;

            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

        } else if (j["type"] == "handle_new_friend_files") {

            std::string friend_UID = j["friend_UID"].get<std::string>();
            std::string file_name = j["file_name"].get<std::string>();
            
            std::string notification = "好友" + redisManager.get_username(friend_UID) + "(UID为:" + friend_UID + ")" + "给你发来了文件 " + file_name;
            redisManager.delete_notification(j["UID"], "file", notification);

        } else if (j["type"] == "recv_file") {
            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd,j);
            });

            pool.add_task([this, connected_sockfd, file_name = j["file_name"]] {
                do_send_file(connected_sockfd, file_name);
            });


        } else if (j["type"] == "send_group_file") {
            //将数据发送回原客户端
            pool.add_task([this, connected_sockfd, j] {
                do_send(connected_sockfd, j);
            });
            
            //设置为阻塞模式
            int flags = fcntl(connected_sockfd, F_GETFL, 0);
            if (flags < 0) {
                std::cerr << "fcntl(F_GETFL) failed: " << strerror(errno) << std::endl;
                return;
            }
            flags &= ~O_NONBLOCK;
            if (fcntl(connected_sockfd, F_SETFL, flags) < 0) {
                std::cerr << "fcntl(F_SETFL) failed: " << strerror(errno) << std::endl;
                return;
            }

            //取消监视
            epoll_ctl(epfd, EPOLL_CTL_DEL, connected_sockfd, NULL);

            pool.add_task([this, fd = connected_sockfd, UID = j["UID"], GID = j["GID"]] {
                do_recv_group_file(fd, UID, GID);
            });

            //直接返回，要读取文件了，不再读取json数据
            return;

        } else if (j["type"] == "") {

        } else if (j["type"] == "") {

        } else if (j["type"] == "") {

        }
    }
}

void Server::do_recv_friend_file(int connected_sockfd, std::string UID, std::string friend_UID) {
    //接收文件
    struct len_name ln;
    char lnbuf[1024];
    char file_path[1024];
    char buf[1024];

    read(connected_sockfd, lnbuf, 1024);
    memcpy(&ln, lnbuf, sizeof(ln));

    // LogInfo("len = {}", ln.len);

    sprintf(file_path, "../file_buf/%s", ln.name);

    FILE *fp = fopen(file_path, "wb");
    if (fp == NULL) {
        perror("Can't open file");
        exit(1);
    }

    ssize_t n;
    unsigned long sum = 0;
    while((n = read(connected_sockfd, buf, 1024)) > 0) {
        fwrite(buf, sizeof(char), n, fp);

        sum += n;
        // LogInfo("sum = {}", sum);
        if(sum >= ln.len)	//当接收到足够的长度的时候，就说明文件已读取完毕
        {
            break;
        }
    }

    fclose(fp);

    LogInfo("接收文件完成");

    //存储消息通知到redis
    std::string notification = "好友" + redisManager.get_username(UID) + "(UID为:" + UID + ")" + "给你发来了文件 " + ln.name;

    std::vector<std::string> notifications;
    redisManager.get_notification(friend_UID, "file", notifications);

    //如果重复就删除以前的消息通知保留最新的
    for (const auto& n : notifications) {
        if (notification == n) {
            redisManager.delete_notification(friend_UID, "file", n);
        }
    }

    //存储消息通知
    redisManager.add_notification(friend_UID, "file", notification);

    //查询用户是否在线
    auto it = map.find(friend_UID);

    //在线
    if (it != map.end()) {
        json j;
        j["type"] = "notice";
        j["file_notification"] = 1;

        int connected_sockfd2 = it->second;

        pool.add_task([this, connected_sockfd2, j] {
            do_send(connected_sockfd2, j);
        });
    }

    //设置套接字为非阻塞模式
    int flags = fcntl(connected_sockfd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl(F_GETFL) failed" << std::endl;
        return;
    }
    if (fcntl(connected_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl(F_SETFL) failed" << std::endl;
    }
    
    //重新监视
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, connected_sockfd, &event2) < 0) {
        throw std::runtime_error("Failed to add socket to epoll");
    }
}

void Server::do_recv_group_file(int connected_sockfd, std::string UID, std::string GID) {
    //接收文件
    struct len_name ln;
    char lnbuf[1024];
    char file_path[1024];
    char buf[1024];

    read(connected_sockfd, lnbuf, 1024);
    memcpy(&ln, lnbuf, sizeof(ln));

    // LogInfo("len = {}", ln.len);

    sprintf(file_path, "../file_buf/%s", ln.name);

    FILE *fp = fopen(file_path, "wb");
    if (fp == NULL) {
        perror("Can't open file");
        exit(1);
    }

    ssize_t n;
    unsigned long sum = 0;
    while((n = read(connected_sockfd, buf, 1024)) > 0) {
        fwrite(buf, sizeof(char), n, fp);

        sum += n;
        // LogInfo("sum = {}", sum);
        if(sum >= ln.len)	//当接收到足够的长度的时候，就说明文件已读取完毕
        {
            break;
        }
    }

    fclose(fp);

    LogInfo("接收文件完成");

    //存储消息通知到redis
    std::string notification = "群聊" + redisManager.get_group_name(GID) + "(GID为:" + GID + ")" + "发来了文件 " + ln.name;

    if (redisManager.get_group_owner_UID(GID) != UID) {
        std::vector<std::string> notifications;
        redisManager.get_notification(redisManager.get_group_owner_UID(GID), "file", notifications);

        //如果重复就删除以前的消息通知保留最新的
        for (const auto& n : notifications) {
            if (notification == n) {
                redisManager.delete_notification(redisManager.get_group_owner_UID(GID), "file", n);
            }
        }

        redisManager.add_notification(redisManager.get_group_owner_UID(GID), "file", notification);

        //查询用户是否在线
        auto it = map.find(redisManager.get_group_owner_UID(GID));

        //在线
        if (it != map.end()) {

            json j2;
            j2["type"] = "notice";
            j2["file_notification"] = 1;

            int connected_sockfd2 = it->second;

            pool.add_task([this, connected_sockfd2, j2] {
                do_send(connected_sockfd2, j2);
            });
        
        }
    }

    std::vector<std::string> administrators_UID, members_UID;
    redisManager.get_administrators(GID, administrators_UID);
    redisManager.get_group_members(GID, members_UID);

    for (int i = 0; i < administrators_UID.size(); ++i) {

        if (UID != administrators_UID[i]) {
            std::vector<std::string> notifications;
            redisManager.get_notification(administrators_UID[i], "file", notifications);

            //如果重复就删除以前的消息通知保留最新的
            for (const auto& n : notifications) {
                if (notification == n) {
                    redisManager.delete_notification(administrators_UID[i], "file", n);
                }
            }

            redisManager.add_notification(administrators_UID[i], "file", notification);

            //查询用户是否在线
            auto it = map.find(administrators_UID[i]);

            //在线
            if (it != map.end()) {

                json j2;
                j2["type"] = "notice";
                j2["file_notification"] = 1;

                int connected_sockfd2 = it->second;

                pool.add_task([this, connected_sockfd2, j2] {
                    do_send(connected_sockfd2, j2);
                });
            
            }
        }

    }

    for (int i = 0; i < members_UID.size(); ++i) {

        if (UID != members_UID[i]) {
            std::vector<std::string> notifications;
            redisManager.get_notification(members_UID[i], "file", notifications);

            //如果重复就删除以前的消息通知保留最新的
            for (const auto& n : notifications) {
                if (notification == n) {
                    redisManager.delete_notification(members_UID[i], "file", n);
                }
            }

            redisManager.add_notification(members_UID[i], "file", notification);

            //查询用户是否在线
            auto it = map.find(members_UID[i]);

            //在线
            if (it != map.end()) {

                json j2;
                j2["type"] = "notice";
                j2["file_notification"] = 1;

                int connected_sockfd2 = it->second;

                pool.add_task([this, connected_sockfd2, j2] {
                    do_send(connected_sockfd2, j2);
                });
            
            }
        }

    }

    //设置套接字为非阻塞模式
    int flags = fcntl(connected_sockfd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl(F_GETFL) failed" << std::endl;
        return;
    }
    if (fcntl(connected_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl(F_SETFL) failed" << std::endl;
    }
    
    //重新监视
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, connected_sockfd, &event2) < 0) {
        throw std::runtime_error("Failed to add socket to epoll");
    }
}

void Server::do_send(int connected_sockfd, const json& j) {
    //将JSON对象序列化为字符串
    std::string json_str = j.dump(); 

    //存储发送消息的相关信息的结构体
    struct msghdr msg = {0};

    //描述数据缓冲区的结构体
    struct iovec iov[1];

    //存储数据
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

    //发送数据
    ssize_t sent_len = sendmsg(connected_sockfd, &msg, 0);
    if (sent_len < 0) {
        perror("sendmsg");
        return;
    }
}

void Server::do_send_file(int connected_sockfd, std::string file_name) {
    char file_path[1024];
    char buf[1024];
    struct stat statbuf;
    struct len_name ln;

    // LogInfo("file_name = {}", file_name);

    sprintf(file_path, "/home/pluto/work/Chatroom/chatroom/Server/file_buf/%s", file_name.c_str());
    // LogInfo("file_path = {}", file_path);

    if(stat(file_path, &statbuf) == -1)
    {
        std::cerr << "Error getting file status: " << strerror(errno) << std::endl;
    }

    ln.len = statbuf.st_size;
    // LogInfo("len = {}", ln.len);
    strcpy(ln.name, file_name.c_str());

    memcpy(buf, &ln, sizeof(ln));
    write(connected_sockfd, buf, 1024);

    int fp = open(file_path, O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR);
    if (fp == -1) {
        // 打开失败，打印错误信息
        perror("Error opening file");
        return;
    }

    off_t offset = 0;
    ssize_t total_bytes_sent = 0;
    ssize_t bytes_sent;

    while (total_bytes_sent < ln.len) {
        bytes_sent = sendfile(connected_sockfd, fp, &offset, ln.len - total_bytes_sent);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                perror("sendfile");
                break;
            }
        }
        if (bytes_sent == 0) {
            // 可能是连接关闭或没有更多数据可发送
            break;
        }
        total_bytes_sent += bytes_sent;
    }

    // LogInfo("sendfile前len = {}", ln.len);
    // ssize_t tem = sendfile(connected_sockfd, fp, 0, ln.len);
    // LogInfo("sendfile 返回值 = {}", tem);
    //返回值检测

    close(fp);

    LogInfo("发送文件完成");
}

//专门负责检测客户端是否连接
// void Server::heartbeat(int connected_sockfd) {
//     json j;

//     struct msghdr msg = {0};
//     struct iovec iov[1];
//     char buf[1024] = {0};

//     iov[0].iov_base = buf;
//     iov[0].iov_len = sizeof(buf) - 1;
//     msg.msg_iov = iov;
//     msg.msg_iovlen = 1;

//     ssize_t received_len = recvmsg(connected_sockfd, &msg, 0);
//     if (received_len < 0) {
//         perror("recvmsg");
//         return;
//     }

//     if (received_len == 0 || buf[0] == '\0') {
//         return; // 继续等待下一个数据
//     }

//     buf[received_len] = '\0'; // 确保字符串结束符

//     // LogInfo("buf: {}", buf);

//     try {
//         j = json::parse(buf); // 解析 JSON 字符串
//     } catch (const std::exception& e) {
//         std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
//         return;
//     }

//     if (j["type"] == "heartbeat") {
        
//     }
// }