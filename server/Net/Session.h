#pragma once

#include <WinSock2.h>
#include <mutex>
#include <string>
#include <thread>
#include <functional>
#include <cstdint>

#include "../../Common/Protocol.h"
#include "../Core/Clock.h"

class Room;

const int BUFFER_SIZE = 4096;

class Session
{
public:
    SOCKET sock;
    char recvBuf[BUFFER_SIZE];
    int recvLen;

    std::string userName;
    bool isLoggedIn = false;

    int  playerID = 0;
    Room* currentRoom = nullptr;
    int64_t lastHeartbeatUs = 0;

    std::mutex sendMutex;
    std::function<void()> onDisconnect;
    std::function<void(int playerID)> onJoinRoom;
    std::function<void(int playerID)> onLeaveRoom;
    std::function<bool(int playerID, const std::string& username)> onTryReconnect;

public:
    Session(SOCKET s);
    void StartRecv();
    void Send(MsgID id, const void* body, uint16_t bodyLen);
};

void RecvThread(Session* session);
void ProcessMsg(Session* session, MsgID msgId, const char* body, int bodyLen);
