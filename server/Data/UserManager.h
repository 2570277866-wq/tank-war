#pragma once

#include <string>
#include <vector>

struct UserStats {
    std::string account;
    int wins;
    int losses;
    int kills;
};

// 注册新用户，账号已存在返回 false
bool Register(const std::string& account, const std::string& pwd);

// 登录校验，账号不存在或密码错误返回 false
bool Login(const std::string& account, const std::string& pwd);

// 更新战绩：wins/losses/kills 为增量值
bool UpdateStats(const std::string& account, int addWins, int addLosses, int addKills);

// 获取所有用户战绩（用于排行榜）
std::vector<UserStats> GetAllUsers();
