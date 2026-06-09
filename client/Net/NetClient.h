#pragma once

#include "Protocol.h"
#include <WinSock2.h>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdint>

struct QueuedMsg {
    MsgID              id;
    std::vector<char>  body;
};

class NetClient {
public:
    NetClient();
    ~NetClient();

    bool connect(const char* ip, uint16_t port);
    void disconnect();
    bool isConnected() const { return m_connected && m_sock != INVALID_SOCKET; }

    void sendMsg(MsgID id, const char* body, uint16_t bodyLen);
    void sendMsg(MsgID id);

    void poll();

    void setOnMsgCallback(std::function<void(MsgID, const char*, uint16_t)> cb) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_onMsg = std::move(cb);
    }

private:
    void recvThread();

    SOCKET      m_sock = INVALID_SOCKET;
    std::atomic<bool> m_connected{false};
    std::thread m_recvThread;

    char        m_recvBuf[4096];
    int         m_recvLen = 0;

    std::mutex  m_sendMutex;

    // 消息队列：recv 线程写入，主线程 poll() 消费
    std::mutex              m_queueMutex;
    std::vector<QueuedMsg>  m_msgQueue;

    std::mutex  m_callbackMutex;
    std::function<void(MsgID, const char*, uint16_t)> m_onMsg;
};
