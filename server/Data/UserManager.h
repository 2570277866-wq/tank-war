#pragma once

#include <fstream>
#include <string>

using namespace std;

// 全局配置
// 用户数据存储文件名
const char *USER_FILE = "users.txt";

// 保存用户数据到文件
void SaveUser(const string &account, const string &pwd, const string &name)
{
    // 以追加模式打开文件
    ofstream ofs(USER_FILE, ios::app);
    if (ofs)
    {
        ofs << account << " " << pwd << " " << name << endl;
    }
}