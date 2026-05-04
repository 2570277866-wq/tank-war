#pragma once

#include "../../Common/Protocol.h"
#include <WinSock2.h>
#include <string>
#include <functional>
#include <mutex>
#include <thread>

class NetClient {
public:
    NetClient();
    ~NetClient();

    bool connect(const char* ip, uint16_t port);
    void disconnect();
    bool isConnected() const { return m_connected; }

    void sendMsg(MsgID id, const char* body, uint16_t bodyLen);
    void sendMsg(MsgID id);

    void poll();

    void setOnMsgCallback(std::function<void(MsgID, const char*, uint16_t)> cb) {
        m_onMsg = cb;
    }

private:
    void recvThread();
    void processBuffer();

    SOCKET      m_sock = INVALID_SOCKET;
    bool        m_connected = false;
    std::thread m_recvThread;

    char        m_recvBuf[4096];
    int         m_recvLen = 0;

    std::mutex  m_sendMutex;

    std::function<void(MsgID, const char*, uint16_t)> m_onMsg;
};
