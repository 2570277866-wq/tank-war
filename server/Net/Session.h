#pragma once

#include <WinSock2.h>
#include <queue>
#include <mutex>
#include <string>
#include <thread>
#include <functional>
#include "Protocol.h"

const int BUFFER_SIZE = 4096;

class Session
{
public:
    SOCKET sock;               // 客户端的连接socket
    char recvBuf[BUFFER_SIZE]; // 接收缓冲区
    int recvLen;               // 当前收到多少数据

    std::queue<std::string> sendQueue; // 待发送消息队列
    std::mutex sendMutex;              // 防止多线程冲突

    std::function<void()> onDisconnect; // 断线回调

public:
    Session(SOCKET s); // 创建Session
    void StartRecv();
};

// 接收线程（独立线程运行）
void RecvThread(Session *session);
// 处理一条完整消息
void ProcessMsg(Session *session, const char *data, int len);
