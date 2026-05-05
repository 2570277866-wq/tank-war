#pragma once

#include <WinSock2.h>
#include <mutex>
#include <string>
#include <thread>
#include <functional>

#include "../../Common/Protocol.h"

const int BUFFER_SIZE = 4096;

class Session
{
public:
    SOCKET sock;
    char recvBuf[BUFFER_SIZE];  // 接收缓冲区
    int recvLen;                // 缓冲区中已接收的数据长度

    // 用户绑定（登录后设置）
    std::string userName;
    bool isLoggedIn = false;

    std::mutex sendMutex;                // 保护 send() 调用
    std::function<void()> onDisconnect;  // 断线回调，通知 TCPServer 清理

    // 房间操作回调（由 TCPServer 在创建 Session 时绑定）
    std::function<void(int playerID)> onJoinRoom;
    std::function<void(int playerID)> onLeaveRoom;

public:
    Session(SOCKET s);
    void StartRecv();

    // 发送消息：自动组装 MsgHeader + body
    void Send(MsgID id, const void* body, uint16_t bodyLen);
};

// 接收线程（独立线程运行）
void RecvThread(Session* session);

// 消息分发：根据 msgId 调用不同的处理函数
void ProcessMsg(Session* session, MsgID msgId, const char* body, int bodyLen);
