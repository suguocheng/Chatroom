#include "Account.h"
#include "../Home/Home.h"

int main() {
    main_menu_UI();
    return 0;
}

void main_menu_UI() {
    int n;
    while (1) {
        system("clear");
        std::cout << "欢迎进入聊天室！" << std::endl;
        std::cout << "1.登录" << std::endl;
        std::cout << "2.注册" << std::endl;
        std::cout << "3.找回密码" << std::endl;
        std::cout << "4.退出" << std::endl;
        std::cout << "请输入：";
        std::cin >> n;
        if (n == 1) {
            log_in_UI();
        } else if (n == 2) {
            sign_up_UI();
        } else if (n == 3) {
            //邮箱验证接口
        } else if (n == 4) {
            exit(0);
        } else {
            std::cout << "请正确输入选项！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
}

void log_in_UI() {
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

        if (accountManager.log_in(username, password)==1) {
            home_UI(username);
            return;
        } else {
            return;
        }
    }
}

void sign_up_UI() {
    while (1) {
        system("clear");
        std::cout << "注册" << std::endl;

        AccountManager accountManager("127.0.0.1", 6379);
        std::string username;
        std::string password;
        std::string email;

        std::cout << "请输入你的用户名：";
        std::cin >> username;
        std::cout << "请输入你的密码：";
        std::cin >> password;
        std::cout << "请输入你的电子邮箱：";
        std::cin >> email;

        accountManager.sign_up(username, password, email);
        return;
    }
}

void change_usename_UI(std::string oldUsername) {
    while (1) {
        system("clear");
        std::cout << "更改密码" << std::endl;

        AccountManager accountManager("127.0.0.1", 6379);
        std::string newUsername;

        std::cout << "请输入你的新用户名：";
        std::cin >> newUsername;

        accountManager.change_username(oldUsername, newUsername);
        return;
    }
}

void change_emaill_UI(std::string username) {
    while (1) {
        system("clear");
        std::cout << "更改电子邮箱" << std::endl;

        AccountManager accountManager("127.0.0.1", 6379);
        std::string password;
        std::string newEmail;

        std::cout << "请输入你的密码：";
        std::cin >> password;
        std::cout << "请输入你的新电子邮箱：";
        std::cin >> password;

        accountManager.change_email(username, password, newEmail);
        return;
    }
}

void change_password_UI(std::string username) {
    while (1) {
        system("clear");
        std::cout << "更改密码" << std::endl;

        AccountManager accountManager("127.0.0.1", 6379);
        std::string oldPassword;
        std::string newPassword;

        std::cout << "请输入你的旧密码：";
        std::cin >> oldPassword;
        std::cout << "请输入你的新密码：";
        std::cin >> newPassword;

        accountManager.change_email(username, oldPassword, newPassword);
        return;
    }
}

void log_out_UI(std::string username) {
    while (1) {
        system("clear");
        std::cout << "注销帐号" << std::endl;

        AccountManager accountManager("127.0.0.1", 6379);
        std::string password1;
        std::string password2;

        std::cout << "请输入你的密码：";
        std::cin >> password1;
        std::cout << "确认密码：";
        std::cin >> password2;

        accountManager.log_out(password1, password2);
        return;
    }
}

