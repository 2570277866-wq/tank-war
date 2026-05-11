#pragma once

#include <WinSock2.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

#include "../../Common/Protocol.h"
#include "../Game/Room.h"

class Session;

class TCPServer
{
private:
    SOCKET listenSocket;
    std::vector<SOCKET> clients;
    std::mutex mtx;
    std::atomic<bool> running{false};

    std::vector<Room*> rooms;
    std::mutex roomMutex;

    std::unordered_map<int, Session*> sessionMap;
    std::mutex sessionMapMutex;

    std::thread tickThread;
    void GameTickLoop();

    void SendToPlayer(int playerID, MsgID id, const void* body, uint16_t len);

public:
    ~TCPServer();

    bool Init();
    void Start();
    void Shutdown();

    void AcceptLoop();
    void RemoveClient(SOCKET s);

    void RegisterSession(int playerID, Session* s);
    void UnregisterSession(int playerID);

    Room* FindOrCreateRoom();
    void RemoveFromRoom(int playerID);
    void HandleJoinRoom(int playerID, const std::string& username);
    void HandleLeaveRoom(int playerID);
    bool HandleReconnect(int newPlayerID, const std::string& username, Session* session);
};
