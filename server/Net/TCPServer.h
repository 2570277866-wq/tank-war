#pragma once

#include <WinSock2.h>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <algorithm>
#include <functional>

#include "../../Common/Protocol.h"
#include "../Game/Room.h"

// 服务端监听端口（与 Common/Protocol.h 的 SERVER_PORT 一致）
const int PORT = 9527;

class TCPServer
{
private:
    SOCKET listenSocket;
    std::vector<SOCKET> clients;
    std::mutex mtx;
    std::atomic<bool> running{false};

    // 房间管理
    std::vector<Room> rooms;
    std::mutex roomMutex;

public:
    ~TCPServer();

    // 初始化 Winsock、绑定、监听
    bool Init();

    // 启动服务端（开启 Accept 线程）
    void Start();

    // 停止服务端
    void Shutdown();

    // 接收客户端连接循环
    void AcceptLoop();

    // 从客户端列表中移除
    void RemoveClient(SOCKET s);

    // 房间操作
    Room* FindOrCreateRoom();
    void RemoveFromRoom(int playerID);
    void HandleJoinRoom(int playerID);
    void HandleLeaveRoom(int playerID);
};
