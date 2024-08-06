#include "Account_UI.h"
#include "../log/mars_logger.h"

std::string current_UID = "";

void main_menu_UI(int connecting_sockfd) {
    int n;
    while (1) {
        system("clear");
        std::cout << "欢迎进入聊天室！" << std::endl;
        std::cout << "1. 登录" << std::endl;
        std::cout << "2. 注册" << std::endl;
        std::cout << "3. 找回密码" << std::endl;
        std::cout << "4. 退出" << std::endl;
        std::cout << "请输入：";

        // 读取用户输入
        if (!(std::cin >> n)) {
            std::cin.clear(); // 清除错误标志
            std::cout << "无效的输入，请输入一个数字！" << std::endl;
            waiting_for_input();
            continue; // 重新显示菜单
        }

        switch (n) {
            case 1:
                log_in_UI(connecting_sockfd);
                // LogInfo("2.current_UID = {}", current_UID);
                if(current_UID != "") {
                    usleep(50000);
                    waiting_for_input();
                    home_UI(connecting_sockfd, current_UID);
                }
                break;
            case 2:
                sign_up_UI(connecting_sockfd);
                break;
            case 3:
                retrieve_password(connecting_sockfd);
                break;
            case 4:
                exit(0);
            default:
                std::cout << "请正确输入选项！" << std::endl;
                break;
        }
        waiting_for_input();
    }
}

void log_in_UI(int connecting_sockfd) {
    system("clear");
    std::cout << "登录" << std::endl;

    json j;
    j["type"] = "log_in";
    std::string username, password;

    std::cout << "请输入你的用户名：";
    std::cin >> username;
    j["username"] = username;

    std::cout << "请输入你的密码：";
    std::cin >> password;
    j["password"] = password;

    send_json(connecting_sockfd, j);
    usleep(50000);
}

void sign_up_UI(int connecting_sockfd) {
    system("clear");
    std::cout << "注册" << std::endl;

    json j;
    j["type"] = "sign_up";
    std::string username, password, security_question, security_answer;

    std::cout << "请输入你的用户名：";
    std::cin >> username;
    j["username"] = username;

    std::cout << "请输入你的密码：";
    std::cin >> password;
    j["password"] = password;

    std::cout << "请输入你的密保问题：";
    std::cin >> security_question;
    j["security_question"] = security_question;

    std::cout << "请输入你的密保答案：";
    std::cin >> security_answer;
    j["security_answer"] = security_answer;

    send_json(connecting_sockfd, j);
    usleep(50000);
}

void retrieve_password(int connecting_sockfd) {
    system("clear");
    std::cout << "找回密码" << std::endl;

    json j;
    j["type"] = "retrieve_password";
    std::string username;

    std::cout << "请输入你的用户名：";
    std::cin.ignore(); // 清理输入流，以防止之前输入的换行符干扰
    std::cin >> username;
    j["username"] = username;

    send_json(connecting_sockfd, j);
    usleep(50000);

    json j2;
    j2["type"] = "retrieve_password_confirm_answer";
    std::string security_answer;
    
    j2["username"] = username;

    std::cout << "请输入你的密保答案：";
    std::cin >> security_answer;
    j2["security_answer"] = security_answer;

    send_json(connecting_sockfd, j2);
    usleep(50000);
}

void change_usename_UI(int connecting_sockfd, std::string UID) {
    system("clear");
    std::cout << "更改用户名" << std::endl;

    json j;
    j["type"] = "change_usename";
    std::string new_username;

    j["UID"] = UID;

    std::cout << "请输入你的新用户名：";
    std::cin >> new_username;
    j["new_username"] = new_username;

    send_json(connecting_sockfd, j);
    
    usleep(50000);
    waiting_for_input();
}

void change_password_UI(int connecting_sockfd, std::string UID) {
    system("clear");
    std::cout << "更改密码" << std::endl;

    json j;
    j["type"] = "change_password";
    std::string old_password, new_password;

    j["UID"] = UID;

    std::cout << "请输入你的旧密码：";
    std::cin >> old_password;
    j["old_password"] = old_password;

    std::cout << "请输入你的新密码：";
    std::cin >> new_password;
    j["new_password"] = new_password;

    send_json(connecting_sockfd, j);

    usleep(50000);
    waiting_for_input();
}

void change_security_question_UI(int connecting_sockfd, std::string UID) {
    system("clear");
    std::cout << "更改密保问题" << std::endl;

    json j;
    j["type"] = "change_security_question";
    std::string password, new_security_question, new_security_answer;

    j["UID"] = UID;

    std::cout << "请输入你的密码：";
    std::cin >> password;
    j["password"] = password;

    std::cout << "请输入你的新密保问题：";
    std::cin >> new_security_question;
    j["new_security_question"] = new_security_question;

    // LogInfo("new_security_question = {}", (j["new_security_question"]));
    
    std::cout << "请输入你的新密保答案：";
    std::cin >> new_security_answer;
    j["new_security_answer"] = new_security_answer;

    // LogInfo("new_security_answer = {}", (j["new_security_answer"]));

    send_json(connecting_sockfd, j);

    usleep(50000);
    waiting_for_input();
}

void log_out_UI(int connecting_sockfd, std::string UID) {
    
    system("clear");
    std::cout << "注销帐号" << std::endl;

    json j;
    j["type"] = "log_out";
    std::string password, confirm_password;

    j["UID"] = UID;

    std::cout << "请输入你的密码：";
    std::cin >> password;
    j["password"] = password;

    std::cout << "请确认你的密码：";
    std::cin >> confirm_password;
    j["confirm_password"] = confirm_password;

    if(j["password"] != j["confirm_password"]) {
        std::cout << "两次密码不一致" << std::endl;
        waiting_for_input();
        return;
    }

    send_json(connecting_sockfd, j);

    usleep(50000);
    waiting_for_input();
}

