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

    //日志输出启动服务器
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

                //日志输出连接到客户端
                LogInfo("成功连接到客户端!");
                // LogInfo("connected_socked = {}", connected_sockfd);

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

                //如果监控到读事件就将接收函数添加到线程池运行
                pool.add_task([this, fd = events[i].data.fd] {
                    // LogTrace("something read happened");
                    do_recv(fd);
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
            perror("recvmsg");
            return;
        }

        buf[received_len] = '\0'; // 确保字符串结束符
        
        //如果没有读取到数据，return继续等待下一次读取
        if (received_len == 0 || buf[0] == '\0') {
            return; 
        }

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
                        
                        //只要登录就发送粗略的消息通知
                        // json j2;
                        // std::vector<std::string> notifications;
                        // j2["type"] = "notice";
                        // j2["friend_request_notification"] = redisManager.get_notification(j["UID"], "friend_request", notifications);
                        // j2["group_request_notification"] = redisManager.get_notification(j["UID"], "group_request", notifications);
                        // j2["message_notification"] = redisManager.get_notification(j["UID"], "message", notifications);
                        
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

            LogInfo("准备添加好友");

            if (j["request_UID"] == 0) {
                continue;
            }

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
            redisManager.get_chat_messages(j["UID"], j["friend_UID"], messages);

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
                if (redisManager.add_chat_message(j["UID"], j["friend_UID"], j["message"]) != 1) {
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
            
        } else if (j["type"] == "") {
            
        } else if (j["type"] == "") {
            
        } else if (j["type"] == "") {
            
        }
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

//专门负责检测客户端是否连接
void Server::heartbeat(int connected_sockfd) {
    json j;

    struct msghdr msg = {0};
    struct iovec iov[1];
    char buf[1024] = {0};

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf) - 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    ssize_t received_len = recvmsg(connected_sockfd, &msg, 0);
    if (received_len < 0) {
        perror("recvmsg");
        return;
    }

    if (received_len == 0 || buf[0] == '\0') {
        return; // 继续等待下一个数据
    }

    buf[received_len] = '\0'; // 确保字符串结束符

    // LogInfo("buf: {}", buf);

    try {
        j = json::parse(buf); // 解析 JSON 字符串
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return;
    }

    if (j["type"] == "heartbeat") {
        
    }
}