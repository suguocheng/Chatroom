#include "Account.h"

int main() {
    redisContext* c = redisConnect("127.0.0.1", 6379);

    std::string username;
    std::string password;

    std::cout << "请输入你的用户名：";
    std::cin >> username;
    std::cout << "请输入你的密码：";
    std::cin >> password;

    redisReply* reply = (redisReply*)redisCommand(c, "GET %s", username.c_str());
    if (reply->type == REDIS_REPLY_STRING) {
        if (password == reply->str) {
            std::cout << "登录成功！" << std::endl;
            //调用接口
        } else {
            std::cout << "密码错误！" << std::endl;
            //调用接口
        }
    } else {
        std::cout << "不存在该用户！" << std::endl;
        //调用接口
    }
    freeReplyObject(reply);

    redisFree(c);
    return 0;
}