#include "NetClient.h"
#include <cstring>
#include <iostream>

NetClient::NetClient() {
    memset(m_recvBuf, 0, sizeof(m_recvBuf));
}

NetClient::~NetClient() {
    disconnect();
}

bool NetClient::connect(const char* ip, uint16_t port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[NetClient] WSAStartup failed" << std::endl;
        return false;
    }

    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        std::cerr << "[NetClient] socket failed" << std::endl;
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (::connect(m_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[NetClient] connect failed" << std::endl;
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    m_connected = true;
    m_recvLen = 0;

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_msgQueue.clear();
    }

    m_recvThread = std::thread(&NetClient::recvThread, this);

    std::cout << "[NetClient] connected to " << ip << ":" << port << std::endl;
    return true;
}

void NetClient::disconnect() {
    m_connected = false;
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
    if (m_recvThread.joinable()) {
        m_recvThread.join();
    }
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_msgQueue.clear();
    }
    WSACleanup();
    std::cout << "[NetClient] disconnected" << std::endl;
}

void NetClient::sendMsg(MsgID id, const char* body, uint16_t bodyLen) {
    if (!m_connected) return;

    char buf[4096];
    MsgHeader header;
    header.id = id;
    header.bodyLen = bodyLen;

    memcpy(buf, &header, sizeof(MsgHeader));
    if (bodyLen > 0 && body) {
        memcpy(buf + sizeof(MsgHeader), body, bodyLen);
    }

    std::lock_guard<std::mutex> lock(m_sendMutex);

    // TCP 可能部分发送，循环确保全部发送
    int totalLen = (int)sizeof(MsgHeader) + (int)bodyLen;
    int sent = 0;
    while (sent < totalLen) {
        int ret = send(m_sock, buf + sent, totalLen - sent, 0);
        if (ret <= 0) return;
        sent += ret;
    }
}

void NetClient::sendMsg(MsgID id) {
    sendMsg(id, nullptr, 0);
}

void NetClient::recvThread() {
    char tmpBuf[4096];

    while (m_connected) {
        int ret = recv(m_sock, tmpBuf, sizeof(tmpBuf), 0);
        if (ret <= 0) {
            m_connected = false;
            std::cout << "[NetClient] server disconnected" << std::endl;
            break;
        }

        int totalLen = m_recvLen + ret;
        if (totalLen > (int)sizeof(m_recvBuf)) {
            m_recvLen = 0;
            totalLen = ret;
        }

        memcpy(m_recvBuf + m_recvLen, tmpBuf, ret);
        m_recvLen = totalLen;

        // 解析完整消息 → 入队
        while (m_recvLen >= (int)sizeof(MsgHeader)) {
            MsgHeader header;
            memcpy(&header, m_recvBuf, sizeof(MsgHeader));

            uint32_t msgTotal = sizeof(MsgHeader) + header.bodyLen;
            if (m_recvLen < (int)msgTotal) break;

            // 入队（vector 装任意大小）
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                QueuedMsg qm;
                qm.id = header.id;
                if (header.bodyLen > 0) {
                    qm.body.assign(m_recvBuf + sizeof(MsgHeader),
                                   m_recvBuf + sizeof(MsgHeader) + header.bodyLen);
                }
                m_msgQueue.push_back(std::move(qm));
            }

            m_recvLen -= (int)msgTotal;
            if (m_recvLen > 0) {
                memmove(m_recvBuf, m_recvBuf + msgTotal, m_recvLen);
            }
        }
    }
}

void NetClient::poll() {
    // 在主线程安全地消费队列中的消息
    std::vector<QueuedMsg> pending;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        pending.swap(m_msgQueue);
    }

    std::function<void(MsgID, const char*, uint16_t)> cb;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        cb = m_onMsg;
    }

    if (cb) {
        for (const auto& qm : pending) {
            cb(qm.id, qm.body.data(), (uint16_t)qm.body.size());
        }
    }
}
