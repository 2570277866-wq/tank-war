#pragma once

#include <fstream>
#include <string>

// 全局配置
// 用户数据存储文件名
const char *USER_FILE = "users.txt";

// 保存用户数据到文件
inline void SaveUser(const std::string &account, const std::string &pwd, const std::string &name)
{
    // 以追加模式打开文件
    std::ofstream ofs(USER_FILE, std::ios::app);
    if (ofs)
    {
        ofs << account << " " << pwd << " " << name << std::endl;
    }
}