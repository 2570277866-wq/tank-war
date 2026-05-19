#include "UserManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>

using namespace std;

static const char* USER_FILE = "users.dat";
static const char* USER_TMP  = "users.tmp";

// DJB2 哈希
static string HashPwd(const string& pwd)
{
    unsigned int h = 5381;
    for (char c : pwd)
        h = ((h << 5) + h) + c;
    stringstream ss;
    ss << hex << setfill('0') << setw(8) << h;
    return ss.str();
}

bool Register(const string& account, const string& pwd)
{
    ifstream ifs(USER_FILE);
    string line;
    while (getline(ifs, line))
    {
        size_t pos = line.find('|');
        if (pos != string::npos && line.substr(0, pos) == account)
            return false;
    }
    ifs.close();

    ofstream ofs(USER_FILE, ios::app);
    if (!ofs) return false;
    ofs << account << "|" << HashPwd(pwd) << "|0|0|0" << endl;
    return true;
}

bool Login(const string& account, const string& pwd)
{
    ifstream ifs(USER_FILE);
    if (!ifs) return false;

    string line;
    string hash = HashPwd(pwd);
    while (getline(ifs, line))
    {
        size_t p1 = line.find('|');
        if (p1 == string::npos) continue;
        if (line.substr(0, p1) != account) continue;

        size_t p2 = line.find('|', p1 + 1);
        if (p2 == string::npos) continue;
        string storedHash = line.substr(p1 + 1, p2 - p1 - 1);

        return storedHash == hash;
    }
    return false;
}

bool UpdateStats(const string& account, int addWins, int addLosses, int addKills)
{
    ifstream ifs(USER_FILE);
    if (!ifs) return false;

    ofstream ofs(USER_TMP);
    if (!ofs) return false;

    bool found = false;
    string line;
    while (getline(ifs, line))
    {
        size_t p1 = line.find('|');
        if (p1 == string::npos) { ofs << line << endl; continue; }
        if (line.substr(0, p1) != account) { ofs << line << endl; continue; }

        size_t p2 = line.find('|', p1 + 1);
        if (p2 == string::npos) { ofs << line << endl; continue; }
        size_t p3 = line.find('|', p2 + 1);
        if (p3 == string::npos) { ofs << line << endl; continue; }
        size_t p4 = line.find('|', p3 + 1);
        if (p4 == string::npos) { ofs << line << endl; continue; }

        int wins   = stoi(line.substr(p2 + 1, p3 - p2 - 1)) + addWins;
        int losses = stoi(line.substr(p3 + 1, p4 - p3 - 1)) + addLosses;
        int kills  = stoi(line.substr(p4 + 1)) + addKills;

        ofs << account << "|" << line.substr(p1 + 1, p2 - p1 - 1)
            << "|" << wins << "|" << losses << "|" << kills << endl;
        found = true;
    }

    ifs.close();
    ofs.close();

    if (!found) return false;

    remove(USER_FILE);
    rename(USER_TMP, USER_FILE);
    return true;
}

vector<UserStats> GetAllUsers()
{
    vector<UserStats> users;
    ifstream ifs(USER_FILE);
    if (!ifs) return users;

    string line;
    while (getline(ifs, line))
    {
        size_t p1 = line.find('|');
        if (p1 == string::npos) continue;
        size_t p2 = line.find('|', p1 + 1);
        if (p2 == string::npos) continue;
        size_t p3 = line.find('|', p2 + 1);
        if (p3 == string::npos) continue;
        size_t p4 = line.find('|', p3 + 1);
        if (p4 == string::npos) continue;

        UserStats u;
        u.account = line.substr(0, p1);
        u.wins   = stoi(line.substr(p2 + 1, p3 - p2 - 1));
        u.losses = stoi(line.substr(p3 + 1, p4 - p3 - 1));
        u.kills  = stoi(line.substr(p4 + 1));
        users.push_back(u);
    }
    return users;
}
