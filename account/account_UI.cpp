#include "Account.h"

int main() {
    MainMenuUI();
    return 0;
}

void MainMenuUI() {
    int n;
    while (1) {
        system("clear");
        std::cout << "欢迎进入聊天室！" << std::endl;
        std::cout << "1.登录" << std::endl;
        std::cout << "2.注册" << std::endl;
        std::cout << "3.退出" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            LogInUI();
        } else if(n == 2) {
            SignUpUI();
        } else if(n == 3) {
            exit(0);
        } else {
            std::cout << "请正确输入选项！" << std::endl;
        }
    }
}

void LogInUI() {
    while (1) {
        system("clear");
        std::cout << "登录" << std::endl;

        AccountManager accountManager("127.0.0.1", 6379);
        std::string username;
        std::string password;

        std::cout << "请输入你的用户名：";
        std::cin >> username;
        std::cout << "请输入你的密码：";
        std::cin >> password;

        if (accountManager.login(username, password)==1) {
            //调用
        } else {
            MainMenuUI();
        }

    }
}

void SignUpUI() {
    while (1) {
        system("clear");
        std::cout << "注册" << std::endl;

        AccountManager accountManager("127.0.0.1", 6379);
        std::string username;
        std::string password;

        std::cout << "请输入你的用户名：";
        std::cin >> username;
        std::cout << "请输入你的密码：";
        std::cin >> password;

        accountManager.signup(username, password);
        MainMenuUI();
    }
}