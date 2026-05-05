#pragma once

#include <string>

// 注册新用户，账号已存在返回 false
bool Register(const std::string& account, const std::string& pwd);

// 登录校验，账号不存在或密码错误返回 false
bool Login(const std::string& account, const std::string& pwd);
