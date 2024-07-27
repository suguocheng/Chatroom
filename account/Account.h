#include <iostream>
#include <unordered_map>
#include <string>
#include <hiredis/hiredis.h>

void MainMenuUI();
void LogInUI();
void SignUpUI();

class AccountManager {
public:
    AccountManager(const std::string& host, int port);
    ~AccountManager();
    
    bool login(const std::string& username, const std::string& password);
    bool signup(const std::string& username, const std::string& password);

private:
    redisContext* redisContext_;
};