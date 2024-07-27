#include "Account.h"

int main() {
    redisContext* c = redisConnect("127.0.0.1", 6379);

    std::unordered_map<std::string, std::string> account;
    std::string username;
    std::string password;

    std::cout << "请输入你的用户名：";
    std::cin >> username;
    std::cout << "请输入你的密码：";
    std::cin >> password;

    account[username]=password;
    
    auto it = account.find(username);
    if (it != account.end()) {
        redisReply* reply = (redisReply*)redisCommand(c, "SET %s %s", it->first.c_str(), it->second.c_str());
        if (reply == NULL) {
            std::cerr << "Redis command failed" << std::endl;
        } else {
            std::cout << "注册成功！" << std::endl;
            //调用接口
            freeReplyObject(reply);
        }
    }

    redisFree(c);
    return 0;
}