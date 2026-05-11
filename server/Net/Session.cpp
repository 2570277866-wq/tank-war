#include "Session.h"
#include "../Data/UserManager.h"
#include <iostream>
#include <cstring>

using namespace std;

Session::Session(SOCKET s) : sock(s), recvLen(0)
{
    memset(recvBuf, 0, BUFFER_SIZE);
}

// 发送一条完整消息 = MsgHeader + body
void Session::Send(MsgID id, const void* body, uint16_t bodyLen)
{
    lock_guard<mutex> lock(sendMutex);

    MsgHeader header;
    header.id = id;
    header.bodyLen = bodyLen;

    // 发送消息头
    int ret = send(sock, (const char*)&header, sizeof(header), 0);
    if (ret <= 0) return;

    // 发送消息体（可能为空）
    if (body && bodyLen > 0)
        send(sock, (const char*)body, bodyLen, 0);
}

// ============ 消息处理 ============

// 处理注册：C2S_REGISTER
static void HandleRegister(Session* session, const char* body, int bodyLen)
{
    // 消息体格式：account\0password\0
    string account(body);
    string pwd(body + account.size() + 1);

    cout << "[服务端] 注册请求：账号=" << account << endl;

    bool ok = Register(account, pwd);
    ErrorCode code = ok ? ErrorCode::NONE : ErrorCode::REGISTER_EXISTS;

    // 回复注册结果
    session->Send(MsgID::S2C_REGISTER_ACK, &code, sizeof(code));
    cout << "[服务端] 注册" << (ok ? "成功" : "失败（账号已存在）") << endl;
}

// 处理登录：C2S_LOGIN
static void HandleLogin(Session* session, const char* body, int bodyLen)
{
    // 消息体格式：account\0password\0
    string account(body);
    string pwd(body + account.size() + 1);

    cout << "[服务端] 登录请求：账号=" << account << endl;

    bool ok = Login(account, pwd);
    ErrorCode code = ok ? ErrorCode::NONE : ErrorCode::LOGIN_FAILED;

    if (ok)
    {
        session->userName = account;
        session->isLoggedIn = true;
    }

    // 回复登录结果
    session->Send(MsgID::S2C_LOGIN_ACK, &code, sizeof(code));
    cout << "[服务端] 登录" << (ok ? "成功" : "失败") << endl;
}

// 消息分发器：根据 msgId 调用对应的处理函数
void ProcessMsg(Session* session, MsgID msgId, const char* body, int bodyLen)
{
    switch (msgId)
    {
    case MsgID::C2S_REGISTER:
        HandleRegister(session, body, bodyLen);
        break;

    case MsgID::C2S_LOGIN:
        HandleLogin(session, body, bodyLen);
        break;

    case MsgID::C2S_JOIN_ROOM:
        if (session->onJoinRoom)
            session->onJoinRoom((int)session->sock);
        break;

    case MsgID::C2S_LEAVE_ROOM:
        if (session->onLeaveRoom)
            session->onLeaveRoom((int)session->sock);
        break;

    default:
        cout << "[服务端] 未知消息类型：msgId=0x" << hex << (uint16_t)msgId << dec << endl;
        break;
    }
}

// ============ 接收线程 ============

void RecvThread(Session* session)
{
    char tempBuf[BUFFER_SIZE];

    while (true)
    {
        // 接收客户端数据
        int ret = recv(session->sock, tempBuf, BUFFER_SIZE, 0);
        if (ret <= 0)
            break; // 连接断开

        // 缓冲区溢出检查
        if (session->recvLen + ret > BUFFER_SIZE)
        {
            cout << "[服务端] 缓冲区溢出，强制断开连接" << endl;
            break;
        }

        // 拷贝到会话缓冲区
        memcpy(session->recvBuf + session->recvLen, tempBuf, ret);
        session->recvLen += ret;

        // 消息分包处理：循环解析完整消息
        while (session->recvLen >= (int)sizeof(MsgHeader))
        {
            MsgHeader header;
            memcpy(&header, session->recvBuf, sizeof(MsgHeader));

            // 计算一条完整消息的总长度
            unsigned int totalLen = sizeof(MsgHeader) + header.bodyLen;
            if (session->recvLen < (int)totalLen)
                break; // 数据不够，等待下一包

            // 一条完整消息已收到，分发处理
            ProcessMsg(session, header.id,
                       session->recvBuf + sizeof(MsgHeader),
                       header.bodyLen);

            // 把缓冲区剩余数据前移
            session->recvLen -= (int)totalLen;
            memmove(session->recvBuf, session->recvBuf + totalLen, session->recvLen);
        }
    }

    // 客户端断开，清理资源
    if (session->onDisconnect)
        session->onDisconnect();
    closesocket(session->sock);
    cout << "[服务端] 客户端断开连接" << endl;
    delete session;
}

void Session::StartRecv()
{
    thread(RecvThread, this).detach();
}
