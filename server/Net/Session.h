#pragma once

#include <WinSock2.h>
#include <queue>
#include <mutex>
#include "Protocol.h"

using namespace std;

// 接收缓冲区大小
const int BUFFER_SIZE = 4096;

// ===================== Session类：每个客户端连接一个对象 =====================
class Session
{
public:
    SOCKET sock;               // 客户端连接socket
    char recvBuf[BUFFER_SIZE]; // 接收缓冲区
    int recvLen;               // 当前缓冲区有效数据长度

    queue<string> sendQueue; // 待发送消息队列
    mutex sendMutex;         // 发送队列锁

public:
    // 构造函数：初始化客户端连接
    Session(SOCKET s);

    // 启动接收线程
    void StartRecv();
};

// 客户端接收线程
void RecvThread(Session *session);

// 处理解析完成的消息
void ProcessMsg(Session *session, const char *data, int len);