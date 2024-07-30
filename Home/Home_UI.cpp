#include "Home.h"
#include "../account/Account.h"

// int main() {
//     Home_UI();
//     return 0;
// }

void home_UI(const std::string& username) {
    int n;
    while (1) {
        system("clear");
        std::cout << "主页" << std::endl;
        std::cout << "1.消息" << std::endl; //这里加个通知
        std::cout << "2.通讯录" << std::endl; //这里加个通知
        std::cout << "3.个人中心" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            message_UI();
        } else if (n == 2) {
            contacts_UI();
        } else if (n == 3) {
            if (user_UI(username)==1) {
                return;
            }
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

void message_UI() {
    char n;
    while (1) {
        system("clear");
        std::cout << "消息" << std::endl;
        //按回复时间输出会话列表
        std::cout << "输入R返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 'r' || n == 'R') {
            return;
        }
    }
}

void contacts_UI() {
    int n;
    while (1) {
        system("clear");
        std::cout << "通讯录" << std::endl;
        std::cout << "1.好友" << std::endl; 
        std::cout << "2.群聊" << std::endl; 
        std::cout << "3.添加好友/群" << std::endl;//这里加个通知
        std::cout << "4.创建群聊" << std::endl;
        std::cout << "5.返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            friends_UI();
        } else if (n == 2) {
            groups_UI();
        } else if (n == 3) {
            add_friends_groups_UI();
        } else if (n == 4) {
            //创建群聊
        } else if (n == 5) {
            return;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

void friends_UI() {
    char n;
    while (1) {
        system("clear");
        std::cout << "好友" << std::endl;
        //按首字母排序输出好友列表
        std::cout << "输入R返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 'r' || n == 'R') {
            return;
        }
    }
}

void groups_UI() {
    char n;
    while (1) {
        system("clear");
        std::cout << "群聊" << std::endl;
        //按首字母排序输出群聊列表
        std::cout << "输入R返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 'r' || n == 'R') {
            return;
        }
    }
}

void add_friends_Groups_UI() {
    int n;
    while (1) {
        system("clear");
        std::cout << "添加好友/群" << std::endl;
        std::cout << "1.添加好友" << std::endl; 
        std::cout << "2.添加群聊" << std::endl;
        std::cout << "3.返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            
        } else if (n == 2) {
            
        } else if (n == 3) {
            return;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

bool user_UI(const std::string& username) {
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
        std::cin >> n;
        if (n == 1) {
            information_UI(username);
        } else if (n == 2) {
            change_password_UI(username);
        } else if (n == 3) {
            return 1;
        } else if (n == 4) {
            log_out_UI(username);
        } else if (n == 5) {
            return 0;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

void information_UI(const std::string& username) {
    int n;
    while (1) {
        system("clear");
        std::cout << "个人信息" << std::endl;
        std::cout << "1.用户名" << std::endl; 
        std::cout << "2.UID" <<  std::endl;
        std::cout << "3.电子邮箱" << std::endl;
        std::cout << "4.返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            username_UI(username);
        } else if (n == 2) {
            AccountManager accountManager("127.0.0.1", 6379);
            system("clear");
            std::cout << "\033[31m" << accountManager.get_UID(username) << "\033[0m" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        } else if (n == 3) {
            email_UI(username);
        } else if (n == 3) {
            return;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

void username_UI(const std::string& username){
    int n;
    while (1) {
        system("clear");
        std::cout << "用户名" << std::endl;
        std::cout << "1.用户名：" << username << std::endl;
        std::cout << "2.更改用户名" << std::endl;
        std::cout << "3.返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            system("clear");
            std::cout << "\033[31m" << username << "\033[0m" <<std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        } else if (n == 2) {
            change_usename_UI(username);
        } else if (n == 3) {
            return;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

void email_UI(const std::string& username) {
    int n;
    AccountManager accountManager("127.0.0.1", 6379);

    while (1) {
        system("clear");
        std::cout << "电子邮箱" << std::endl;
        std::cout << "1.电子邮箱：" << accountManager.get_email(username) << std::endl;
        std::cout << "2.更改电子邮箱" << std::endl;
        std::cout << "3.返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            system("clear");
            std::cout << "\033[31m" << username << "\033[0m" <<std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        } else if (n == 2) {
            change_emaill_UI(username);
        } else if (n == 3) {
            return;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

void add_friends_groups_UI() {

}