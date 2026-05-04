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
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (::connect(m_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[NetClient] connect failed" << std::endl;
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    m_connected = true;
    m_recvLen = 0;
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
    send(m_sock, buf, sizeof(MsgHeader) + bodyLen, 0);
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

        if (m_recvLen + ret > (int)sizeof(m_recvBuf)) {
            m_recvLen = 0;
        }

        memcpy(m_recvBuf + m_recvLen, tmpBuf, ret);
        m_recvLen += ret;

        processBuffer();
    }
}

void NetClient::processBuffer() {
    while (m_recvLen >= (int)sizeof(MsgHeader)) {
        MsgHeader header;
        memcpy(&header, m_recvBuf, sizeof(MsgHeader));

        uint32_t totalLen = sizeof(MsgHeader) + header.bodyLen;
        if (m_recvLen < (int)totalLen) break;

        if (m_onMsg) {
            m_onMsg(header.id, m_recvBuf + sizeof(MsgHeader), header.bodyLen);
        }

        m_recvLen -= totalLen;
        if (m_recvLen > 0) {
            memmove(m_recvBuf, m_recvBuf + totalLen, m_recvLen);
        }
    }
}

void NetClient::poll() {
}
