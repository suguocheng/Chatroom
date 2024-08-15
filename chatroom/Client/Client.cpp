#include "Client.h"
#include "../log/mars_logger.h"

sem_t message_notice_semaphore;
sem_t friend_request_notice_semaphore;
sem_t group_request_notice_semaphore;
sem_t file_notice_semaphore;

Client::Client(int port) : pool(10){

    sem_init(&message_notice_semaphore, 0, 0);
    sem_init(&friend_request_notice_semaphore, 0, 0);
    sem_init(&group_request_notice_semaphore, 0, 0);
    sem_init(&file_notice_semaphore, 0, 0);

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

    pool.add_task([this] {
        do_message_notice();
    });

    pool.add_task([this] {
        do_friend_request_notice();
    });

    pool.add_task([this] {
        do_group_request_notice();
    });

    pool.add_task([this] {
        do_file_notice();
    });

    //将接收函数添加到线程池运行
    pool.add_task([this] {
        do_recv();
    });
    
    //将心跳发送函数添加到线程池运行
    // pool.add_task([this] {
    //     heartbeat();
    // });

    //主菜单，在里面负责发送数据
    main_menu_UI(connecting_sockfd);
}


Client::~Client() {
    close(connecting_sockfd);
    sem_destroy(&semaphore); // 销毁信号量
    pool.~ThreadPool();
}


void Client::do_message_notice() {
    while (1) {
        sem_wait(&message_notice_semaphore); // 等待信号量
        std::cout << std::endl << "\033[31m" << "您收到了新消息" << "\033[0m" << std::endl << std::endl;
    }
}

void Client::do_friend_request_notice() {
    while (1) {
        sem_wait(&friend_request_notice_semaphore); // 等待信号量
        std::cout << std::endl << "\033[31m" << "您收到了新好友申请" << "\033[0m" << std::endl << std::endl;
    }
}

void Client::do_group_request_notice() {
    while (1) {
        sem_wait(&group_request_notice_semaphore); // 等待信号量
        std::cout << std::endl << "\033[31m" << "您收到了新加群申请" << "\033[0m" << std::endl << std::endl;
    }
}

void Client::do_file_notice() {
    while (1) {
        sem_wait(&group_request_notice_semaphore); // 等待信号量
        std::cout << std::endl << "\033[31m" << "您收到了新文件" << "\033[0m" << std::endl << std::endl;
    }
}

void Client::do_recv() {
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
            continue; // 继续等待下一个数据
        }

        // LogInfo("buf = {}", buf);

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
            
            sem_post(&semaphore); // 释放信号量
            // LogInfo("current_UID = {}", current_UID);

        } else if (j["type"] == "sign_up") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "retrieve_password") {
            std::cout << "你的密保问题：" << j["security_question"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "retrieve_password_confirm_answer") {
            std::cout << "你的密码：" << j["password"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "change_usename") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "change_security_question") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "change_password") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "quit_log") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "log_out") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "get_username") {
            std::cout << "用户:" << j["username"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "get_security_question") {
            std::cout << j["security_question"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        //不需要释放信号量，这个没有输出不会造成输出冲突，也没有信号量在等待它
        } else if (j["type"] == "notice") {
            if (j["friend_request_notification"] == 1) {
                notice_map["friend_request_notification"] = 1;
                sem_post(&friend_request_notice_semaphore); // 释放信号量
            } else {
                notice_map["friend_request_notification"] = 0;
            }

            if (j["group_request_notification"] == 1) {
                notice_map["group_request_notification"] = 1;
                sem_post(&group_request_notice_semaphore); // 释放信号量
            } else {
                notice_map["group_request_notification"] = 0;
            }

            if (j["message_notification"] == 1) {
                notice_map["message_notification"] = 1;
                sem_post(&message_notice_semaphore); // 释放信号量
            } else {
                notice_map["message_notification"] = 0;
            }

            if (j["file_notification"] == 1) {
                notice_map["file_notification"] = 1;
                sem_post(&file_notice_semaphore); // 释放信号量
            } else {
                notice_map["file_notification"] = 0;
            }

            // LogInfo("message_notification = {}", (notice_map["message_notification"]));
            
        } else if (j["type"] == "add_friend") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "view_friend_requests") {
            for (const auto& notification : j["friend_requests"]) {
                std::cout << notification << std::endl;
            }
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "agree_to_friend_request") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "view_friends_list") {
            for (int i = 0; i < j["friends_name"].size(); ++i) {
                std::cout << "用户名:" << j["friends_name"][i] << "  UID:" << j["friends_UID"][i] << std::endl;
            }
            sem_post(&semaphore); // 释放信号量
        
        } else if (j["type"] == "confirmed_as_friend") {
            if (j["result"] == "该用户是好友") {
                confirmed_as_friend = 1;
            } else {
                confirmed_as_friend = 0;
            }
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "check_online_status") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "delete_friend") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "block_friend") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "unblock_friend") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "get_chat_messages") {
            for (const auto& message : j["messages"]) {
                std::cout << message << std::endl;
            }
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "send_chat_messages") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "view_messages") {
            for (const auto& message : j["messages"]) {
                std::cout << message << std::endl;
            }
            // int sem_value;
            // sem_getvalue(&semaphore, &sem_value);
            // LogInfo("post前semaphore = {}", sem_value);
            sem_post(&semaphore); // 释放信号量
            // sem_getvalue(&semaphore, &sem_value);
            // LogInfo("post后semaphore = {}", sem_value);

        } else if (j["type"] == "create_group") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "add_group") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "view_group_requests") {
            for (const auto& notification : j["group_requests"]) {
                std::cout << notification << std::endl;
            }
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "agree_to_group_request") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "view_groups_list") {
            for (int i = 0; i < j["groups_name"].size(); ++i) {
                std::cout << "群组名:" << j["groups_name"][i] << "  GID:" << j["groups_GID"][i] << std::endl;
            }
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "confirm_in_group") {
            if (j["result"] == "您在该群组中") {
                confirm_in_group = 1;
            } else {
                confirm_in_group = 0;
            }
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "get_group_name") {
            std::cout << j["result"] << "的群聊" << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "view_member_list") {
            std::cout << "群主:" << std::endl;
            std::cout << "用户名:" << j["group_owner_username"] << "  UID:" << j["group_owner_UID"] << std::endl;

            std::cout << "管理员:" << std::endl;
            for (int i = 0; i < j["administrators_username"].size(); ++i) {
                std::cout << "用户名:" << j["administrators_username"][i] << "  UID:" << j["administrators_UID"][i] << std::endl;
            }

            std::cout << "成员:" << std::endl;
            for (int i = 0; i < j["members_username"].size(); ++i) {
                std::cout << "用户名:" << j["members_username"][i] << "  UID:" << j["members_UID"][i] << std::endl;
            }
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "exit_group") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "dismiss_group") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "remove_group_member") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "set_up_administrator") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "remove_administrator") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "get_group_chat_messages") {
            for (const auto& message : j["messages"]) {
                std::cout << message << std::endl;
            }
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "send_group_chat_messages") {
            std::cout << j["result"] << std::endl;
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "send_friend_file") {
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "confirmed_as_block_friend") {
            if (j["result"] == "该好友已将您屏蔽") {
                confirmed_as_block_friend = 1;
            } else {
                confirmed_as_block_friend = 0;
            }
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "view_file") {
            for (const auto& file : j["files"]) {
                std::cout << file << std::endl;
            }
            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "recv_file") {
            //接收文件
            struct len_name ln;
            char lnbuf[1024];
            char file_path[1024];
            char buf[1024];

            // LogInfo("准备读取文件名和长度");

            read(connecting_sockfd, lnbuf, 1024);
            memcpy(&ln, lnbuf, sizeof(ln));

            printf("\n开始接收文件< %s >,请勿退出!\n", ln.name);

            sprintf(file_path, "../file_buf/%s", ln.name);

            FILE *fp = fopen(file_path, "wb");
            if (fp == NULL) {
                perror("Can't open file");
                exit(1);
            }

            // LogInfo("len = {}", ln.len);

            ssize_t n;
            unsigned long sum = 0;
            while((n = read(connecting_sockfd, buf, 1024)) > 0) {//返回值为0或小于0
                // LogInfo("n = {}", n);
                fwrite(buf, sizeof(char), n, fp);
                
                sum += n;
                // LogInfo("sum = {}", sum);
                if(sum >= ln.len)	//当接收到足够的长度的时候，就说明文件已读取完毕
                {
                    break;
                }
                printProgressBar(sum, ln.len);
            }

            fclose(fp);

            printf("\n文件< %s >接收成功!\n\n", ln.name);

            sem_post(&semaphore); // 释放信号量
            
        } else if (j["type"] == "send_group_file") {
            sem_post(&semaphore); // 释放信号量

        } else if (j["type"] == "") {
            
        } else if (j["type"] == "") {
            
        } else if (j["type"] == "") {
            
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

void Client::do_recv_file() {
    struct len_name ln;
    char lnbuf[1024];
    char file_path[1024];
    char buf[1024];

    read(connecting_sockfd, lnbuf, 1024);
    memcpy(&ln, lnbuf, sizeof(ln));

    sprintf(file_path, "../file_buf/%s", ln.name);

    FILE *fp = fopen(file_path, "wb");
    if (fp == NULL) {
        perror("Can't open file");
        exit(1);
    }

    ssize_t n;
    unsigned int sum = 0;
    while((n = read(connecting_sockfd, buf, 1024)) > 0) {
        // LogInfo("n = ", n);
        fwrite(buf, sizeof(char), n, fp);
        sum += n;
        // LogInfo("sum = ", sum);
        if(sum >= ln.len)	//当接收到足够的长度的时候，就说明文件已读取完毕
        {
            break;
        }
    }

    fclose(fp);
    sem_post(&semaphore); // 释放信号量
}


//这个心跳函数好像会与正常的操作冲突造成异常
// void Client::heartbeat() 
// {
//     while (1) 
//     {
//         std::string msg = "heartbeat";
//         usleep(1000000); // 暂停1秒
//         j["type"] = "heartbeat";
//         j["msg"] = msg;
//         send_json(connecting_sockfd, j);
//     }
// }

//还得有个实时通知功能的线程
