#include "Home.h"

// int main() {
//     Home_UI();
//     return 0;
// }

void Home_UI() {
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
            Message_UI();
        } else if (n == 2) {
            Contacts_UI();
        } else if (n == 3) {
            if (User_UI()==1) {
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

void Message_UI() {
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

void Contacts_UI() {
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
            Friends_UI();
        } else if (n == 2) {
            Group_UI();
        } else if (n == 3) {
            AddFriendsGroup_UI();
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

void Friends_UI() {
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

void Group_UI() {
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

void AddFriendsGroup_UI() {
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

bool User_UI() {
    int n;
    while (1) {
        system("clear");
        std::cout << "个人中心" << std::endl;
        std::cout << "1.退出帐号" << std::endl; 
        std::cout << "2.注销帐号" << std::endl;
        std::cout << "3.返回" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            return 1;
        } else if (n == 2) {
            
        } else if (n == 3) {
            return 0;
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}