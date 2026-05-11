#include "TCPServer.h"
#include "Session.h"
#include <iostream>

using namespace std;

TCPServer::~TCPServer()
{
    Shutdown();
}

bool TCPServer::Init()
{
    // 初始化 Windows 网络库 Winsock
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0)
    {
        cerr << "[服务端] WSAStartup 失败，错误码：" << err << endl;
        return false;
    }

    // 创建监听 socket（流式 TCP）
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        cerr << "[服务端] 创建 socket 失败，错误码：" << WSAGetLastError() << endl;
        WSACleanup();
        return false;
    }

    // 绑定 IP 和端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        cerr << "[服务端] 绑定端口 " << PORT << " 失败，错误码：" << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    // 开始监听
    if (listen(listenSocket, 100) == SOCKET_ERROR)
    {
        cerr << "[服务端] 监听失败，错误码：" << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    cout << "=====================================" << endl;
    cout << "          TCP 服务端已启动" << endl;
    cout << "          监听端口：" << PORT << endl;
    cout << "=====================================" << endl;

    return true;
}

void TCPServer::Start()
{
    running = true;
    thread(&TCPServer::AcceptLoop, this).detach();
}

void TCPServer::Shutdown()
{
    if (!running.exchange(false))
        return;

    closesocket(listenSocket);

    // 关闭所有客户端连接
    {
        lock_guard<mutex> lock(mtx);
        for (SOCKET s : clients)
            closesocket(s);
        clients.clear();
    }

    WSACleanup();
    cout << "[服务端] 服务端已停止" << endl;
}

void TCPServer::RemoveClient(SOCKET s)
{
    lock_guard<mutex> lock(mtx);
    auto it = find(clients.begin(), clients.end(), s);
    if (it != clients.end())
        clients.erase(it);
}

// 找一个有空位的房间，没有则创建新房间
Room* TCPServer::FindOrCreateRoom()
{
    lock_guard<mutex> lock(roomMutex);
    for (auto& room : rooms)
    {
        if (!room.IsFull())
            return &room;
    }
    // 没有空闲房间，新建一个
    rooms.emplace_back();
    return &rooms.back();
}

void TCPServer::RemoveFromRoom(int playerID)
{
    lock_guard<mutex> lock(roomMutex);
    for (auto& room : rooms)
        room.Leave(playerID);
}

void TCPServer::HandleJoinRoom(int playerID)
{
    Room* room = FindOrCreateRoom();
    bool ok = room->Join(playerID);

    if (ok)
    {
        cout << "[服务端] 玩家 " << playerID << " 加入房间，当前状态="
             << (int)room->state << "，人数=" << (room->IsFull() ? 2 : 1) << endl;
    }

    // TODO: 发送 S2C_ROOM_INFO 给玩家
}

void TCPServer::HandleLeaveRoom(int playerID)
{
    RemoveFromRoom(playerID);
    cout << "[服务端] 玩家 " << playerID << " 离开房间" << endl;

    // TODO: 发送 S2C_ROOM_INFO 给玩家
}

// ============ Accept 循环 ============

void TCPServer::AcceptLoop()
{
    cout << "[服务端] 等待客户端连接..." << endl;

    while (running)
    {
        // 用 select 设置超时，以便检查 running 标志
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(listenSocket, &fds);

        timeval tv{1, 0}; // 1 秒超时
        int ret = select(0, &fds, nullptr, nullptr, &tv);
        if (ret <= 0)
            continue;

        SOCKET client = accept(listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET)
            continue;

        cout << "[服务端] 新客户端连接：socket=" << client << endl;

        // 加入客户端列表
        {
            lock_guard<mutex> lock(mtx);
            clients.push_back(client);
        }

        // 创建 Session，绑定回调
        Session* session = new Session(client);
        session->onDisconnect = [this, client]()
        {
            RemoveClient(client);
            RemoveFromRoom(client);
        };
        // 房间操作回调
        session->onJoinRoom = [this](int pid) { HandleJoinRoom(pid); };
        session->onLeaveRoom = [this](int pid) { HandleLeaveRoom(pid); };

        session->StartRecv();
    }
}
