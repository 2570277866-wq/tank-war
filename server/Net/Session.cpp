#include "Session.h"
#include <iostream>
#include <cstring>

using namespace std;

Session::Session(SOCKET s) : sock(s), recvLen(0)
{
    memset(recvBuf, 0, BUFFER_SIZE); // 缓冲区清零
}

// 处理解析完成的消息
void ProcessMsg(Session *session, const char *data, int len)
{
    // 输出收到的消息（作业展示用）
    cout << "[服务端] 收到消息：" << data << endl;
}

// 每个客户端接收独立线程
void RecvThread(Session *session)
{
    char tempBuf[BUFFER_SIZE];

    while (true)
    {
        // 接收客户端数据
        int ret = recv(session->sock, tempBuf, BUFFER_SIZE, 0);
        if (ret <= 0)
            break; // 连接断开则退出循环

        // 缓冲区溢出检查，丢弃超出容量的数据并断开连接
        if (session->recvLen + ret > BUFFER_SIZE)
        {
            cout << "[服务端] 缓冲区溢出，强制断开连接" << endl;
            break;
        }

        // 将收到的数据拷贝到会话缓冲区
        memcpy(session->recvBuf + session->recvLen, tempBuf, ret);
        session->recvLen += ret;

        // 消息分包处理
        while (session->recvLen >= (int)sizeof(MsgHeader))
        {
            MsgHeader header;
            // 先拷贝出消息头
            memcpy(&header, session->recvBuf, sizeof(MsgHeader));

            // 计算一条完整消息总长度
            unsigned int totalLen = sizeof(MsgHeader) + header.msgLen;
            // 缓冲区数据不够一条完整消息，等待下一次接收
            if (session->recvLen < (int)totalLen)
                break;

            // 一条完整消息已收到 
            // 交给业务逻辑处理
            ProcessMsg(session, session->recvBuf + sizeof(MsgHeader), header.msgLen);

            // 处理完一条消息，把缓冲区剩余数据前移
            session->recvLen -= (int)totalLen;
            memmove(session->recvBuf, session->recvBuf + totalLen, session->recvLen);
        }
    }

    // 客户端断开连接，清理资源缓存
    if (session->onDisconnect)
        session->onDisconnect();
    closesocket(session->sock); // 关闭socket
    cout << "[服务端] 客户端断开连接" << endl;
    delete session; // 释放Session对象
}

void Session::StartRecv()
{
    thread(RecvThread, this).detach();
}
