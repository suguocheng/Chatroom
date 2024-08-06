#include "SendJson.h"

AccountManager::AccountManager(const std::string& host, int port) {
    redisContext_ = redisConnect(host.c_str(), port);
    if (redisContext_ == nullptr || redisContext_->err) {
        std::cerr << "无法连接到 Redis 服务器: " << redisContext_->errstr << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    }
    initializeUserIdCounter();
}

AccountManager::~AccountManager() {
    if (redisContext_) {
        redisFree(redisContext_);
    }
}

void AccountManager::initializeUserIdCounter() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SETNX user_id_counter 1");
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
    } else if (reply->integer == 1) {
        
    }
    freeReplyObject(reply);
}

std::string AccountManager::generateUniqueId() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "INCR user_id_counter");
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }
    std::string userId = reply->str;
    freeReplyObject(reply);
    return userId;
}

bool AccountManager::log_in(const std::string& username, const std::string& password) {
    bool success = false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET user:%s password", get_UID(username).c_str());
    
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << redisContext_->errstr << std::endl;
    } else {
        if (password == reply->str) {
            std::cout << "登录成功！" << std::endl;
        } else {
            std::cout << "密码错误！" << std::endl;
        }
    }
    freeReplyObject(reply);
    
    std::cout << "按任意键继续..." << std::endl;
    std::cin.ignore();
    std::cin.get(); // 等待用户输入

    return success;
}

bool AccountManager::sign_up(const std::string& username, const std::string& password, const std::string& email) {
    bool success = false;

    static int uniqueId = 1;
    std::string UID = std::to_string(uniqueId++);

    redisReply* checkReply = (redisReply*)redisCommand(redisContext_, "HGET username_to_id %s", username.c_str());

    if (checkReply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
    } else {
        if (checkReply->integer == 1) {
            std::cout << "该用户名已注册！" << std::endl;
        } else {
            if (set_UID(username,UID) == 1) {
                redisReply* userInfoReply = (redisReply*)redisCommand(redisContext_, 
                "HMSET user:%s password %s email %s", 
                UID.c_str(), password.c_str(), email.c_str());

                if (userInfoReply == NULL) {
                    std::cerr << "Redis 命令执行失败！" << std::endl;
                } else {
                    std::cout << "注册成功！" << std::endl;
                    success = true;
                }
                freeReplyObject(userInfoReply);
            }
        }
    }
    freeReplyObject(checkReply);

    std::cout << "按任意键继续..." << std::endl;
    std::cin.ignore();
    std::cin.get(); // 等待用户输入

    return success;
}

std::string AccountManager::get_UID(const std::string& username) {
    std::string UID;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET username_to_id %s", username.c_str());

    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    } else {
        if (reply->type != REDIS_REPLY_STRING) {
            std::cout << "用户名不存在！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        } else {
            UID = reply->str;
        }
    }
    freeReplyObject(reply);

    return UID;
}

bool AccountManager::set_UID(const std::string& username, const std::string& UID) {
    bool success = false;

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET username_to_id %s %s", username.c_str(), UID.c_str());

    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    } else {
        success = true;
    }
    freeReplyObject(reply);

    return success;
}

std::string AccountManager::get_email(const std::string& username) {
    std::string email;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET user:%s email", get_UID(username).c_str());

    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    } else {    
        email = reply->str;
    }
    freeReplyObject(reply);

    return email;
}

bool AccountManager::change_username(const std::string& oldUsername, const std::string& newUsername) {
    bool success = false;

    redisReply* checkNewUsernameReply = (redisReply*)redisCommand(redisContext_, "HGET username_to_id %s", newUsername.c_str());

    if (checkNewUsernameReply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    } else {
        if (checkNewUsernameReply->type == REDIS_REPLY_STRING) {
            std::cout << "该用户名已被注册！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        } else {
            std::string UID = get_UID(oldUsername);
            redisReply* deleteOldUsernameReply = (redisReply*)redisCommand(redisContext_, "HDEL username_to_id %s", oldUsername.c_str());

            if (deleteOldUsernameReply == NULL) {
                std::cerr << "Redis 命令执行失败！" << std::endl;
                std::cout << "按任意键继续..." << std::endl;
                std::cin.ignore();
                std::cin.get(); // 等待用户输入
            } else {
                if (set_UID(newUsername,UID) == 1) {
                    std::cerr << "用户名更改成功！" << std::endl;
                    std::cout << "按任意键继续..." << std::endl;
                    std::cin.ignore();
                    std::cin.get(); // 等待用户输入
                    success = true;
                }
            }
        }
    }
    freeReplyObject(checkNewUsernameReply);

    return success;
}

bool AccountManager::change_email(const std::string& username, const std::string& password, const std::string& newEmail) {
    bool success = false;

    redisReply* checkReply = (redisReply*)redisCommand(redisContext_, "HGET user:%s password", get_UID(username).c_str());

    if (checkReply == NULL) {
        std::cerr << "Redis 命令执行失败！" << redisContext_->errstr << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    } else {
        if (password == checkReply->str) {
            redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET user:%s email %s", get_UID(username).c_str(), newEmail.c_str());

            if (reply == NULL) {
                std::cerr << "Redis 命令执行失败！" << std::endl;
                std::cout << "按任意键继续..." << std::endl;
                std::cin.ignore();
                std::cin.get(); // 等待用户输入
            } else {
                if (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK") {
                    std::cout << "电子邮箱更改成功！" << std::endl;
                    std::cout << "按任意键继续..." << std::endl;
                    std::cin.ignore();
                    std::cin.get(); // 等待用户输入
                    success = true;
                } else {
                    std::cerr << "电子邮箱更改失败！" << std::endl;
                    std::cout << "按任意键继续..." << std::endl;
                    std::cin.ignore();
                    std::cin.get(); // 等待用户输入
                }   
            }
            freeReplyObject(reply);
        } else {
            std::cout << "密码错误！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
    freeReplyObject(checkReply);

    return success;
}

bool AccountManager::change_password(const std::string& username, const std::string& oldPassword, const std::string& newPassword) {
    bool success = false;

    redisReply* checkReply = (redisReply*)redisCommand(redisContext_, "HGET user:%s password", get_UID(username).c_str());

    if (checkReply == NULL) {
        std::cerr << "Redis 命令执行失败！" << redisContext_->errstr << std::endl;
        std::cout << "按任意键继续..." << std::endl;
        std::cin.ignore();
        std::cin.get(); // 等待用户输入
    } else {
        if (oldPassword == checkReply->str) {
            redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET user:%s password %s", get_UID(username).c_str(), newPassword.c_str());

            if (reply == NULL) {
                std::cerr << "Redis 命令执行失败！" << std::endl;
                std::cout << "按任意键继续..." << std::endl;
                std::cin.ignore();
                std::cin.get(); // 等待用户输入
            } else {
                if (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK") {
                    std::cout << "密码更改成功！" << std::endl;
                    std::cout << "按任意键继续..." << std::endl;
                    std::cin.ignore();
                    std::cin.get(); // 等待用户输入
                    success = true;
                } else {
                    std::cerr << "密码更改失败！" << std::endl;
                    std::cout << "按任意键继续..." << std::endl;
                    std::cin.ignore();
                    std::cin.get(); // 等待用户输入
                }   
            }
            freeReplyObject(reply);
        } else {
            std::cout << "密码错误！" << std::endl;
            std::cout << "按任意键继续..." << std::endl;
            std::cin.ignore();
            std::cin.get(); // 等待用户输入
        }
    }
    freeReplyObject(checkReply);

    return success;
}

bool AccountManager::log_out(const std::string& password1, const std::string& password2) {
    return 1;
}