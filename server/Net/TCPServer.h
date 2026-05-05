#pragma once

#include <WinSock2.h>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <algorithm>

// 服务端监听端口号（项目约定：9527）
const int PORT = 9527;

class TCPServer
{
private:
    SOCKET listenSocket;
    std::vector<SOCKET> clients;
    std::mutex mtx;
    std::atomic<bool> running{false};

public:
    ~TCPServer();

    // 初始化Winsock、绑定、监听
    bool Init();

    // 启动服务端（开线程Accept）
    void Start();

    // 停止服务端
    void Shutdown();

    // 接收客户端连接循环
    void AcceptLoop();

    // 从列表中移除已断开的客户端
    void RemoveClient(SOCKET s);
};
