#include "UserManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

static const char* USER_FILE = "users.dat";

// 简单密码哈希（仅作演示，非安全用途）
// 使用 DJB2 哈希算法，结果转为8位十六进制字符串
static string HashPwd(const string& pwd)
{
    unsigned int h = 5381;
    for (char c : pwd)
        h = ((h << 5) + h) + c; // h * 33 + c
    stringstream ss;
    ss << hex << setfill('0') << setw(8) << h;
    return ss.str();
}

bool Register(const string& account, const string& pwd)
{
    // 检查账号是否已存在
    ifstream ifs(USER_FILE);
    string line;
    while (getline(ifs, line))
    {
        size_t pos = line.find('|');
        if (pos != string::npos && line.substr(0, pos) == account)
            return false; // 账号已存在
    }
    ifs.close();

    // 写入新用户，初始战绩全为 0
    ofstream ofs(USER_FILE, ios::app);
    if (!ofs) return false;
    ofs << account << "|" << HashPwd(pwd) << "|0|0|0" << endl;
    return true;
}

bool Login(const string& account, const string& pwd)
{
    ifstream ifs(USER_FILE);
    if (!ifs) return false; // 文件不存在，无人注册过

    string line;
    string hash = HashPwd(pwd);
    while (getline(ifs, line))
    {
        // 格式：account|pwd_hash|wins|losses|kills
        size_t p1 = line.find('|');
        if (p1 == string::npos) continue;
        if (line.substr(0, p1) != account) continue;

        size_t p2 = line.find('|', p1 + 1);
        if (p2 == string::npos) continue;
        string storedHash = line.substr(p1 + 1, p2 - p1 - 1);

        return storedHash == hash; // 比对哈希值
    }
    return false; // 账号不存在
}
