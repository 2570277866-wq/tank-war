#pragma once

#include <WinSock2.h>
#include <vector>
#include <mutex>
#include <thread>

using namespace std;

// 服务端监听端口号（项目约定：9527）
const int PORT = 9527;

class TCPServer
{
private:
    SOCKET listenSocket;
    vector<SOCKET> clients;
    mutex mtx;

public:
    // 初始化Winsock、绑定、监听
    bool Init();

    // 启动服务端（开线程Accept）
    void Start();

    // 接收客户端连接循环
    void AcceptLoop();
};