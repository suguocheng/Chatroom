#include "Home_UI.h"
#include "../log/mars_logger.h"

std::unordered_map<std::string, int> notice_map;
bool confirmed_as_friend = 0;
bool confirm_in_group = 0;

void home_UI(int connecting_sockfd, std::string UID) {
    int n;
    while (1) {
        system("clear");
        std::cout << "主页" << std::endl;
        std::cout << "1.消息";
        //这里加个通知
        if(notice_map["message_notification"] == 1) {
            std::cout << "(有新消息)" << std::endl;
        } else {
            std::cout << "\n";
        }
        std::cout << "2.通讯录";

        // LogInfo("friend_request_notification = {}", (notice_map["friend_request_notification"]));

        //这里加个通知
        if(notice_map["friend_request_notification"] == 1 || notice_map["group_request_notification"] == 1) {
            std::cout << "(有新申请)" << std::endl;
        } else {
            std::cout << "\n";
        }
        std::cout << "3.个人中心" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        if (n == 1) {
            message_UI(connecting_sockfd, UID);
        } else if (n == 2) {
            contacts_UI(connecting_sockfd, UID);
        } else if (n == 3) {
            if (user_UI(connecting_sockfd, UID) == 1) {
                return;
            }
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();
        }
    }
    
}

void message_UI(int connecting_sockfd, std::string UID) {
    int n;

    //取消消息通知
    notice_map["message_notification"] = 0;

    while (1) {
        system("clear");
        std::cout << "消息" << std::endl << std::endl;

        json j;
        j["type"] = "view_messages";
        j["UID"] = UID;

        //这里有问题，有时候不等待了(解决了)
        // int sem_value;
        // sem_getvalue(&semaphore, &sem_value);

        send_json(connecting_sockfd, j);
        // LogInfo("wait前semaphore = {}", sem_value);
        sem_wait(&semaphore); // 等待信号量
        // sem_getvalue(&semaphore, &sem_value);
        // LogInfo("wait后semaphore = {}", sem_value);

        std::cout << std::endl;
        std::cout << "输入1处理好友消息,输入2处理群聊消息(输入0返回):";

        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        if (n == 1) {
            json j2;
            j2["type"] = "handle_new_friend_messages";
            j2["UID"] = UID;

            std::string friend_UID;

            std::cout << "请输入你想要处理消息的好友的UID(输入0返回):";
            std::cin >> friend_UID;

            j2["friend_UID"] = friend_UID;

            if (friend_UID == "0") {
                return;
            }

            //无需等待信号量，因为不需要返回输出
            send_json(connecting_sockfd, j2);

            private_chat(connecting_sockfd, UID, friend_UID);

        } else if (n == 2) {
            json j2;
            j2["type"] = "handle_new_group_messages";
            j2["UID"] = UID;

            std::string GID;

            std::cout << "请输入你想要处理消息的群聊的GID(输入0返回):";
            std::cin >> GID;

            j2["GID"] = GID;

            if (GID == "0") {
                return;
            }

            //无需等待信号量，因为不需要返回输出
            send_json(connecting_sockfd, j2);

            //进入群聊界面时，不输入东西退出就会删通知，输入了退出就不删了，难办
            
            group_chat(connecting_sockfd, UID, GID);

        } else if (n == 0) {
            return;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();
        }
    }
}

void contacts_UI(int connecting_sockfd, std::string UID) {
    int n;
    while (1) {
        system("clear");
        std::cout << "通讯录" << std::endl;
        std::cout << "1.好友" << std::endl; 
        std::cout << "2.群聊" << std::endl; 
        std::cout << "3.添加好友/群" << std::endl;
        std::cout << "4.好友申请";

        //这里加个通知
        if(notice_map["friend_request_notification"] == 1) {
            std::cout << "(有新申请)" << std::endl;
        } else {
            std::cout << "\n";
        }

        std::cout << "5.加群申请";

        if(notice_map["group_request_notification"] == 1) {
            std::cout << "(有新申请)" << std::endl;
        } else {
            std::cout << "\n";
        }

        std::cout << "6.创建群组" << std::endl;
        std::cout << "7.返回" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        if (n == 1) {
            friends_UI(connecting_sockfd, UID);

        } else if (n == 2) {
            groups_UI(connecting_sockfd, UID);

        } else if (n == 3) {
            add_friends_groups_UI(connecting_sockfd, UID);

        } else if (n == 4) {
            friends_request_UI(connecting_sockfd, UID);

        } else if (n == 5) {
            groups_request_UI(connecting_sockfd, UID);

        } else if (n == 6) {
            create_group_UI(connecting_sockfd, UID);

        } else if (n == 7) {
            return;

        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();
        }
    }
}

void friends_UI(int connecting_sockfd, std::string UID) {
    std::string friend_UID;
    while (1) {
        system("clear");
        std::cout << "好友" << std::endl << std::endl;
        
        json j;
        j["type"] = "view_friends_list";
        j["UID"] = UID;

        send_json(connecting_sockfd, j);
        sem_wait(&semaphore); // 等待信号量

        std::cout << std::endl;

        std::cout << "请输入想查看的好友的UID(输入0返回):";
        std::cin >> friend_UID;

        if (friend_UID == "0") {
            return;
        } else {
            friend_details_UI(connecting_sockfd, UID, friend_UID);
        }
    }
}

void friend_details_UI(int connecting_sockfd, std::string UID, std::string friend_UID) {
    int n;

    //确认是好友
    json j;
    j["type"] = "confirmed_as_friend";
    j["UID"] = UID;
    j["friend_UID"] = friend_UID;
    
    send_json(connecting_sockfd, j);
    sem_wait(&semaphore); // 等待信号量

    if(confirmed_as_friend != 0) {
        confirmed_as_friend = 0;
        while (1) {
            system("clear");
            std::cout << "好友详情" << std::endl;
            std::cout << "1.好友用户名" << std::endl; 
            std::cout << "2.好友UID" << std::endl; 
            std::cout << "3.好友在线状态" << std::endl;
            std::cout << "4.私聊好友" << std::endl;
            std::cout << "5.屏蔽好友" << std::endl;
            std::cout << "6.解除屏蔽" << std::endl;
            std::cout << "7.删除好友" << std::endl;
            std::cout << "8.返回" << std::endl;
            std::cout << "请输入：";

            // 读取用户输入
            if (!(std::cin >> n)) {
                std::cin.clear(); // 清除错误标志
                std::cout << "无效的输入，请输入一个数字！" << std::endl;
                waiting_for_input();
                continue; // 重新显示菜单
            }

            if (n == 1) {
                system("clear");
                json j2;
                j2["type"] = "get_username";
                j2["UID"] = friend_UID;

                send_json(connecting_sockfd, j2);
                sem_wait(&semaphore); // 等待信号量

                waiting_for_input();

            } else if (n == 2) {
                system("clear");
                std::cout << friend_UID << std::endl;
                waiting_for_input();

            } else if (n == 3) {
                system("clear");
                json j3;
                j3["type"] = "check_online_status";
                j3["UID"] = friend_UID;

                send_json(connecting_sockfd, j3);
                sem_wait(&semaphore); // 等待信号量

                waiting_for_input();

            } else if (n == 4) {
                private_chat(connecting_sockfd, UID, friend_UID);

            } else if (n == 5) {
                char choice;
                while (1) {
                    std::cout << "确认屏蔽该好友吗？(Y/N):";
                    std::cin >> choice;
                    if (choice == 'Y' || choice == 'y') {
                        json j4;
                        j4["type"] = "block_friend";
                        j4["UID"] = UID;
                        j4["friend_UID"] = friend_UID;

                        send_json(connecting_sockfd, j4);
                        sem_wait(&semaphore); // 等待信号量

                        waiting_for_input();
                        break;
                    } else if (choice == 'N' || choice == 'n') {
                        break;
                    } else {
                        std::cout << "请正确输入选项！" << std::endl;
                        waiting_for_input();
                    }
                }

            } else if (n == 6) {
                char choice;
                while (1) {
                    std::cout << "确认解除屏蔽吗？(Y/N):";
                    std::cin >> choice;
                    if (choice == 'Y' || choice == 'y') {
                        json j5;
                        j5["type"] = "unblock_friend";
                        j5["UID"] = UID;
                        j5["friend_UID"] = friend_UID;

                        send_json(connecting_sockfd, j5);
                        sem_wait(&semaphore); // 等待信号量

                        waiting_for_input();
                        break;
                    } else if (choice == 'N' || choice == 'n') {
                        break;
                    } else {
                        std::cout << "请正确输入选项！" << std::endl;
                        waiting_for_input();
                    }
                }

            } else if (n == 7) {
                char choice;
                while (1) {
                    std::cout << "确认删除该好友吗？(Y/N):";
                    std::cin >> choice;
                    if (choice == 'Y' || choice == 'y') {
                        json j6;
                        j6["type"] = "delete_friend";
                        j6["UID"] = UID;
                        j6["friend_UID"] = friend_UID;

                        send_json(connecting_sockfd, j6);
                        sem_wait(&semaphore); // 等待信号量

                        waiting_for_input();
                        return;
                    } else if (choice == 'N' || choice == 'n') {
                        break;
                    } else {
                        std::cout << "请正确输入选项！" << std::endl;
                        waiting_for_input();
                    }
                }

            } else if (n == 8) {
                return;

            } else {
                std::cout << "请正确输入选项！" << std::endl;
                waiting_for_input();
            }
        }
    } else {
        system("clear");
        std::cout << "该用户不是你的好友！" << std::endl;
        waiting_for_input();
    }  
}

void private_chat(int connecting_sockfd, std::string UID, std::string friend_UID) {
    //确认是好友
    json j;
    j["type"] = "confirmed_as_friend";
    j["UID"] = UID;
    j["friend_UID"] = friend_UID;
    
    send_json(connecting_sockfd, j);
    sem_wait(&semaphore); // 等待信号量

    if(confirmed_as_friend != 0) {
        confirmed_as_friend = 0;
        std::cin.ignore();
        while (1) {
            system("clear");
            std::cout << "与";
            json j2;
            j2["type"] = "get_username";
            j2["UID"] = friend_UID;

            send_json(connecting_sockfd, j2);
            sem_wait(&semaphore); // 等待信号量

            std::cout << "私聊" << std::endl << std::endl;

            json j3;
            j3["type"] = "get_chat_messages";
            j3["UID"] = UID;
            j3["friend_UID"] = friend_UID;

            send_json(connecting_sockfd, j3);
            sem_wait(&semaphore); // 等待信号量

            std::cout << std::endl;
            std::cout << "请输入你要发送的消息(输入0退出):";

            json j4;
            std::string message;

            j4["type"] = "send_chat_messages";
            j4["UID"] = UID;
            j4["friend_UID"] = friend_UID;

            std::getline(std::cin, message);
            j4["message"] = message;

            if (message == "0") {
                return;
            }

            send_json(connecting_sockfd, j4);
            sem_wait(&semaphore); // 等待信号量

            std::cout << "按回车键继续..." << std::endl;
            std::cin.get(); // 等待用户输入
        }
    } else {
        system("clear");
        std::cout << "该用户不是你的好友！" << std::endl;
        waiting_for_input();
    }

}

void groups_UI(int connecting_sockfd, std::string UID) {
    std::string GID;
    while (1) {
        system("clear");
        std::cout << "群组" << std::endl << std::endl;
        
        json j;
        j["type"] = "view_groups_list";
        j["UID"] = UID;

        send_json(connecting_sockfd, j);
        sem_wait(&semaphore); // 等待信号量

        std::cout << std::endl;

        std::cout << "请输入想查看的群组的GID(输入0返回):";
        std::cin >> GID;

        if (GID == "0") {
            return;
        } else {
            group_details_UI(connecting_sockfd, UID, GID);
        }
    }
}

void group_details_UI(int connecting_sockfd, std::string UID, std::string GID) {
    int n;

    //确认在群组
    json j;
    j["type"] = "confirm_in_group";
    j["UID"] = UID;
    j["GID"] = GID;

    send_json(connecting_sockfd, j);
    sem_wait(&semaphore); // 等待信号量

    if(confirm_in_group != 0) {
        confirm_in_group = 0;
        while (1) {
            system("clear");
            std::cout << "群组详情" << std::endl;
            std::cout << "1.群组名" << std::endl; 
            std::cout << "2.群组GID" << std::endl; 
            std::cout << "3.群组成员列表" << std::endl;
            std::cout << "4.进入群聊" << std::endl;
            std::cout << "5.设置管理员" << std::endl;
            std::cout << "6.移除管理员" << std::endl;
            std::cout << "7.移除成员" << std::endl;
            std::cout << "8.退出群组" << std::endl;
            std::cout << "9.解散群组" << std::endl;
            std::cout << "10.返回" << std::endl;
            std::cout << "请输入：";

            // 读取用户输入
            if (!(std::cin >> n)) {
                std::cin.clear(); // 清除错误标志
                std::cout << "无效的输入，请输入一个数字！" << std::endl;
                waiting_for_input();
                continue; // 重新显示菜单
            }

            if (n == 1) {
                system("clear");
                json j2;
                j2["type"] = "get_group_name";
                j2["GID"] = GID;

                send_json(connecting_sockfd, j2);
                sem_wait(&semaphore); // 等待信号量

                waiting_for_input();

            } else if (n == 2) {
                system("clear");
                std::cout << GID << std::endl;
                waiting_for_input();

            } else if (n == 3) {
                system("clear");
                json j3;
                j3["type"] = "view_member_list";
                j3["GID"] = GID;

                send_json(connecting_sockfd, j3);
                sem_wait(&semaphore); // 等待信号量

                waiting_for_input();

            } else if (n == 4) {
                group_chat(connecting_sockfd, UID, GID);

            } else if (n == 5) {
                std::string member_UID;
                std::cout << "请输入要设置成管理员的成员UID(输入0返回):";
                std::cin >> member_UID;

                if (member_UID == "0") {
                    break;
                }

                json j5;
                j5["type"] = "set_up_administrator";
                j5["UID"] = UID;
                j5["GID"] = GID;
                j5["member_UID"] = member_UID;

                send_json(connecting_sockfd, j5);
                sem_wait(&semaphore); // 等待信号量

                waiting_for_input();

            } else if (n == 6) {
                std::string member_UID;
                std::cout << "请输入要移除管理员的成员UID(输入0返回):";
                std::cin >> member_UID;

                if (member_UID == "0") {
                    break;
                }

                json j6;
                j6["type"] = "remove_administrator";
                j6["UID"] = UID;
                j6["GID"] = GID;
                j6["member_UID"] = member_UID;

                send_json(connecting_sockfd, j6);
                sem_wait(&semaphore); // 等待信号量

                waiting_for_input();

            } else if (n == 7) {
                std::string member_UID;
                std::cout << "请输入要删除的成员UID(输入0返回):";
                std::cin >> member_UID;

                if (member_UID == "0") {
                    break;
                }

                json j7;
                j7["type"] = "remove_group_member";
                j7["UID"] = UID;
                j7["GID"] = GID;
                j7["member_UID"] = member_UID;

                send_json(connecting_sockfd, j7);
                sem_wait(&semaphore); // 等待信号量

                waiting_for_input();

            } else if (n == 8) {
                char choice;
                while (1) {
                    std::cout << "确认退出该群组吗？(Y/N):";
                    std::cin >> choice;
                    if (choice == 'Y' || choice == 'y') {
                        json j8;
                        j8["type"] = "exit_group";
                        j8["UID"] = UID;
                        j8["GID"] = GID;

                        send_json(connecting_sockfd, j8);
                        sem_wait(&semaphore); // 等待信号量

                        waiting_for_input();
                        return;
                    } else if (choice == 'N' || choice == 'n') {
                        break;

                    } else {
                        std::cout << "请正确输入选项！" << std::endl;
                        waiting_for_input();
                    }
                }
            } else if (n == 9) {


            } else if (n == 10) {
                return;

            } else {
                std::cout << "请正确输入选项！" << std::endl;
                waiting_for_input();
            }
        }
    } else {
        system("clear");
        std::cout << "您不在该群组中！" << std::endl;
        waiting_for_input();
    }
}

void group_chat(int connecting_sockfd, std::string UID, std::string GID) {
    //确认在群组
    json j;
    j["type"] = "confirm_in_group";
    j["UID"] = UID;
    j["GID"] = GID;
    
    send_json(connecting_sockfd, j);
    sem_wait(&semaphore); // 等待信号量

    if(confirm_in_group != 0) {
        confirm_in_group = 0;
        std::cin.ignore();
        while (1) {
            system("clear");
            json j2;
            j2["type"] = "get_group_name";
            j2["GID"] = GID;

            send_json(connecting_sockfd, j2);
            sem_wait(&semaphore); // 等待信号量

            std::cout << std::endl;

            json j3;
            j3["type"] = "get_group_chat_messages";
            j3["GID"] = GID;

            send_json(connecting_sockfd, j3);
            sem_wait(&semaphore); // 等待信号量

            std::cout << std::endl;
            std::cout << "请输入你要发送的消息(输入0退出):";

            json j4;
            std::string message;

            j4["type"] = "send_group_chat_messages";
            j4["UID"] = UID;
            j4["GID"] = GID;

            std::getline(std::cin, message);
            j4["message"] = message;

            if (message == "0") {
                return;
            }

            send_json(connecting_sockfd, j4);
            sem_wait(&semaphore); // 等待信号量

            std::cout << "按回车键继续..." << std::endl;
            std::cin.get(); // 等待用户输入
        }
    } else {
        system("clear");
        std::cout << "您不在该群组中！" << std::endl;
        waiting_for_input();
    }

}

void add_friends_groups_UI(int connecting_sockfd, std::string UID) {
    int n;
    while (1) {
        system("clear");
        std::cout << "添加好友/群" << std::endl;
        std::cout << "1.添加好友" << std::endl; 
        std::cout << "2.添加群聊" << std::endl;
        std::cout << "3.返回" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        if (n == 1) {
            add_friend_UI(connecting_sockfd, UID);
        } else if (n == 2) {
            add_group_UI(connecting_sockfd, UID);
        } else if (n == 3) {
            return;

        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();
        }
    }
}

void add_friend_UI(int connecting_sockfd, std::string UID) {
    system("clear");
    std::cout << "添加好友" << std::endl;

    json j;
    j["type"] = "add_friend";
    std::string search_UID;

    j["UID"] = UID;

    std::cout << "搜索(请输入用户UID):";
    std::cin >> search_UID;
    j["search_UID"] = search_UID;

    send_json(connecting_sockfd, j);
    sem_wait(&semaphore); // 等待信号量

    waiting_for_input();
}

void add_group_UI(int connecting_sockfd, std::string UID) {
    system("clear");
    std::cout << "添加群组" << std::endl;

    json j;
    j["type"] = "add_group";
    std::string search_GID;

    j["UID"] = UID;

    std::cout << "搜索(请输入群组GID):";
    std::cin >> search_GID;
    j["search_GID"] = search_GID;

    send_json(connecting_sockfd, j);

    sem_wait(&semaphore); // 等待信号量
    waiting_for_input();
}

void friends_request_UI(int connecting_sockfd, std::string UID) {
    notice_map["friend_request_notification"] = 0;
    
    while (1) {
        system("clear");
        std::cout << "好友申请" << std::endl << std::endl;

        json j;
        j["type"] = "view_friend_requests";
        j["UID"] = UID;

        send_json(connecting_sockfd, j);
        sem_wait(&semaphore); // 等待信号量

        json j2;
        j2["type"] = "agree_to_friend_request";
        std::string request_UID;

        j2["UID"] = UID;

        std::cout << std::endl;
        std::cout << "请输入你同意申请的用户UID(输入0返回):";
        std::cin >> request_UID;

        if (request_UID == "0") {
            return;
        }

        j2["request_UID"] = request_UID;

        send_json(connecting_sockfd, j2);
        sem_wait(&semaphore); // 等待信号量

        waiting_for_input();
    }
}

void groups_request_UI(int connecting_sockfd, std::string UID) {
    notice_map["group_request_notification"] = 0;
    
    while (1) {
        system("clear");
        std::cout << "加群申请" << std::endl << std::endl;

        json j;
        j["type"] = "view_group_requests";
        j["UID"] = UID;

        send_json(connecting_sockfd, j);
        sem_wait(&semaphore); // 等待信号量

        json j2;
        j2["type"] = "agree_to_group_request";
        std::string request_UID, request_GID;

        j2["UID"] = UID;

        //这里防止一个人申请进入多个群聊，所以要输入GID
        std::cout << std::endl;
        std::cout << "请输入你同意申请的用户UID(输入0返回):";
        std::cin >> request_UID;

        if (request_UID == "0") {
            return;
        }

        std::cout << "请输入你同意用户进入的群组GID:";
        std::cin >> request_GID;

        j2["request_UID"] = request_UID;
        j2["request_GID"] = request_GID;

        send_json(connecting_sockfd, j2);
        sem_wait(&semaphore); // 等待信号量

        waiting_for_input();
    }
}

void create_group_UI(int connecting_sockfd, std::string UID) {
    system("clear");
    std::cout << "创建群组" << std::endl;

    json j;
    j["type"] = "create_group";
    std::string group_name;

    std::cout << "请输入你的群名：";
    std::cin >> group_name;
    j["group_name"] = group_name;

    j["group_owner_UID"] = UID;

    send_json(connecting_sockfd, j);
    sem_wait(&semaphore); // 等待信号量

    waiting_for_input();
}

bool user_UI(int connecting_sockfd, std::string UID) {
    int n;
    while (1) {
        system("clear");
        std::cout << "个人中心" << std::endl;
        std::cout << "1.个人信息" << std::endl;
        std::cout << "2.更改密码" << std::endl;
        std::cout << "3.退出帐号" << std::endl;
        std::cout << "4.注销帐号" << std::endl;
        std::cout << "5.返回" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        if (n == 1) {
            information_UI(connecting_sockfd, UID);

        } else if (n == 2) {
            change_password_UI(connecting_sockfd, UID);

        } else if (n == 3) {
            json j;
            j["type"] = "quit_log";
            j["UID"] = UID;

            send_json(connecting_sockfd, j);
            sem_wait(&semaphore); // 等待信号量

            waiting_for_input();
            return 1;

        } else if (n == 4) {
            log_out_UI(connecting_sockfd, UID);
            return 1;
        } else if (n == 5) {
            return 0;

        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();

        }
    }
}

void information_UI(int connecting_sockfd, std::string UID) {
    int n;
    while (1) {
        system("clear");
        std::cout << "个人信息" << std::endl;
        std::cout << "1.用户名" << std::endl; 
        std::cout << "2.UID" <<  std::endl;
        std::cout << "3.密保问题" << std::endl;
        std::cout << "4.返回" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        if (n == 1) {
            username_UI(connecting_sockfd, UID);

        } else if (n == 2) {
            system("clear");
            std::cout << UID << std::endl;
            waiting_for_input();

        } else if (n == 3) {
            security_question_UI(connecting_sockfd, UID);

        } else if (n == 4) {
            return;

        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();

        }
    }
}

void username_UI(int connecting_sockfd, std::string UID){
    int n;
    while (1) {
        system("clear");
        std::cout << "用户名" << std::endl;
        std::cout << "1.用户名：" << std::endl;
        std::cout << "2.更改用户名" << std::endl;
        std::cout << "3.返回" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        if (n == 1) {
            system("clear");
            json j;
            j["type"] = "get_username";
            j["UID"] = UID;

            send_json(connecting_sockfd, j);
            sem_wait(&semaphore); // 等待信号量

            waiting_for_input();

        } else if (n == 2) {
            change_usename_UI(connecting_sockfd, UID);
            waiting_for_input();

        } else if (n == 3) {
            return;

        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();

        }
    }
}

void security_question_UI(int connecting_sockfd, std::string UID) {
    int n;

    while (1) {
        system("clear");
        std::cout << "密保问题" << std::endl;
        std::cout << "1.密保问题：" << std::endl;
        std::cout << "2.更改密保问题" << std::endl;
        std::cout << "3.返回" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }
        
        if (n == 1) {
            system("clear");
            json j;
            j["type"] = "get_security_question";
            j["UID"] = UID;

            send_json(connecting_sockfd, j);
            sem_wait(&semaphore); // 等待信号量

            waiting_for_input();

        } else if (n == 2) {
            change_security_question_UI(connecting_sockfd, UID);

        } else if (n == 3) {
            return;

        } else {
            std::cout << "请正确输入选项！" << std::endl;
            waiting_for_input();

        }
    }
}

void waiting_for_input() {
    std::cout << "按回车键继续..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get(); // 等待用户输入
}