#include "Account.h"

AccountManager::AccountManager(const std::string& host, int port) {
    redisContext_ = redisConnect(host.c_str(), port);
    if (redisContext_ == nullptr || redisContext_->err) {
        std::cerr << "无法连接到 Redis 服务器: " << redisContext_->errstr << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    }
}

AccountManager::~AccountManager() {
    if (redisContext_) {
        redisFree(redisContext_);
    }
}

bool AccountManager::login(const std::string& username, const std::string& password) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "GET %s", username.c_str());
    bool success = false;
    
    if (reply == nullptr) {
        std::cerr << "Redis 命令执行失败: " << redisContext_->errstr << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
        return false;
    }

    
    if (reply->type == REDIS_REPLY_STRING) {
        if (password == reply->str) {
            std::cout << "登录成功！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
            success = true;
        } else {
            std::cout << "密码错误！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    } else {
        std::cout << "不存在该用户！" << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    }

    freeReplyObject(reply);
    return success;
}

bool AccountManager::signup(const std::string& username, const std::string& password) {
    std::unordered_map<std::string, std::string> account;
    account[username]=password;
    bool success = false;

    auto it = account.find(username);
    if (it != account.end()) {
        redisReply* reply = (redisReply*)redisCommand(redisContext_, "SET %s %s", it->first.c_str(), it->second.c_str());
        if (reply == NULL) {
            std::cerr << "Redis 命令执行失败: " << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        } else {
            std::cout << "注册成功！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
            success = true;
            freeReplyObject(reply);
        }
    }

    return success;
}