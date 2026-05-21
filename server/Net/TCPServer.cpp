#include "TCPServer.h"
#include "Session.h"
#include "../Core/Clock.h"
#include <iostream>
#include <chrono>
#include <algorithm>

using namespace std;

TCPServer::~TCPServer() {
    Shutdown();
}

bool TCPServer::Init() {
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        cerr << "[服务端] WSAStartup 失败，错误码：" << err << endl;
        return false;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cerr << "[服务端] 创建 socket 失败，错误码：" << WSAGetLastError() << endl;
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "[服务端] 绑定端口 " << SERVER_PORT << " 失败，错误码：" << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    if (listen(listenSocket, 100) == SOCKET_ERROR) {
        cerr << "[服务端] 监听失败，错误码：" << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    cout << "=====================================" << endl;
    cout << "          TCP 服务端已启动" << endl;
    cout << "          监听端口：" << SERVER_PORT << endl;
    cout << "=====================================" << endl;

    return true;
}

void TCPServer::Start() {
    running = true;
    thread(&TCPServer::AcceptLoop, this).detach();
    tickThread = thread(&TCPServer::GameTickLoop, this);
    cout << "[服务端] 游戏 tick 循环已启动（" << TICK_RATE << "Hz）" << endl;
}

void TCPServer::Shutdown() {
    if (!running.exchange(false))
        return;

    if (tickThread.joinable()) {
        tickThread.join();
    }

    closesocket(listenSocket);

    {
        lock_guard<mutex> lock(mtx);
        for (SOCKET s : clients)
            closesocket(s);
        clients.clear();
    }

    {
        lock_guard<mutex> lock(roomMutex);
        for (Room* r : rooms) delete r;
        rooms.clear();
    }

    WSACleanup();
    cout << "[服务端] 服务端已停止" << endl;
}

// ============ Session Registry ============

void TCPServer::RegisterSession(int playerID, Session* s) {
    lock_guard<mutex> lock(sessionMapMutex);
    sessionMap[playerID] = s;
}

void TCPServer::UnregisterSession(int playerID) {
    lock_guard<mutex> lock(sessionMapMutex);
    sessionMap.erase(playerID);
}

void TCPServer::SendToPlayer(int playerID, MsgID id, const void* body, uint16_t len) {
    lock_guard<mutex> lock(sessionMapMutex);
    auto it = sessionMap.find(playerID);
    if (it != sessionMap.end()) {
        it->second->Send(id, body, len);
    }
}

// ============ Client Management ============

void TCPServer::RemoveClient(SOCKET s) {
    lock_guard<mutex> lock(mtx);
    auto it = find(clients.begin(), clients.end(), s);
    if (it != clients.end())
        clients.erase(it);
}

// ============ Room Operations ============

Room* TCPServer::FindOrCreateRoom() {
    lock_guard<mutex> lock(roomMutex);
    for (auto* room : rooms) {
        if (!room->IsFull())
            return room;
    }
    rooms.push_back(new Room());
    return rooms.back();
}

void TCPServer::RemoveFromRoom(int playerID) {
    lock_guard<mutex> lock(roomMutex);
    for (auto* room : rooms)
        room->Leave(playerID);
}

void TCPServer::HandleJoinRoom(int playerID, const std::string& username) {
    Room* room = FindOrCreateRoom();

    if (!room->sendToPlayer) {
        room->sendToPlayer = [this](int pid, MsgID id, const void* body, uint16_t len) {
            SendToPlayer(pid, id, body, len);
        };
    }

    bool ok = room->Join(playerID, username);

    {
        lock_guard<mutex> lock(sessionMapMutex);
        auto it = sessionMap.find(playerID);
        if (it != sessionMap.end()) {
            it->second->currentRoom = room;
        }
    }

    if (ok) {
        cout << "[服务端] 玩家 " << username << " 加入房间，状态="
             << (int)room->state << "，人数=" << (room->IsFull() ? 2 : 1) << endl;
        SendToPlayer(playerID, MsgID::S2C_ROOM_INFO, &room->state, sizeof(room->state));
    }
}

void TCPServer::HandleLeaveRoom(int playerID) {
    {
        lock_guard<mutex> lock(sessionMapMutex);
        auto it = sessionMap.find(playerID);
        if (it != sessionMap.end()) {
            it->second->currentRoom = nullptr;
        }
    }
    RemoveFromRoom(playerID);
    cout << "[服务端] 玩家 " << playerID << " 离开房间" << endl;
}

bool TCPServer::HandleReconnect(int newPlayerID, const std::string& username, Session* session) {
    lock_guard<mutex> lock(roomMutex);
    for (auto* room : rooms) {
        if (room->TryReconnect(username, newPlayerID)) {
            session->currentRoom = room;
            RegisterSession(newPlayerID, session);

            ErrorCode code = ErrorCode::NONE;
            session->Send(MsgID::S2C_RECONNECT_ACK, &code, sizeof(code));
            return true;
        }
    }

    ErrorCode code = ErrorCode::KICKED;
    session->Send(MsgID::S2C_RECONNECT_ACK, &code, sizeof(code));
    return false;
}

// ============ Accept Loop ============

void TCPServer::AcceptLoop() {
    cout << "[服务端] 等待客户端连接..." << endl;

    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(listenSocket, &fds);

        timeval tv{1, 0};
        int ret = select(0, &fds, nullptr, nullptr, &tv);
        if (ret <= 0)
            continue;

        SOCKET client = accept(listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET)
            continue;

        cout << "[服务端] 新客户端连接：socket=" << client << endl;

        {
            lock_guard<mutex> lock(mtx);
            clients.push_back(client);
        }

        Session* session = new Session(client);
        session->playerID = (int)client;
        RegisterSession(session->playerID, session);

        session->onDisconnect = [this, client, session]() {
            RemoveClient(client);
            if (session->currentRoom) {
                HandleLeaveRoom(client);
            }
            UnregisterSession(client);
        };

        session->onJoinRoom = [this, session](int pid) {
            HandleJoinRoom(pid, session->userName);
        };

        session->onLeaveRoom = [this](int pid) {
            HandleLeaveRoom(pid);
        };

        session->onTryReconnect = [this, session](int newID, const std::string& user) -> bool {
            return HandleReconnect(newID, user, session);
        };

        session->StartRecv();
    }
}

// ============ Game Tick Loop ============

void TCPServer::GameTickLoop() {
    while (running) {
        int64_t frameStart = Clock::Now();

        {
            lock_guard<mutex> lock(roomMutex);

            for (auto* room : rooms) {
                for (int slot = 0; slot < 2; ++slot) {
                    if (room->disconnectPending[slot].exchange(false)) {
                        room->HandleDisconnect(room->playerIDs[slot]);
                    }
                }

                if (room->state == RoomState::PLAYING || room->state == RoomState::PAUSED) {
                    room->Tick(TICK_INTERVAL / 1000.0f);
                }

                if (room->IsPlaying()) {
                    int64_t now = Clock::Now();
                    for (int slot = 0; slot < 2; ++slot) {
                        int pid = room->playerIDs[slot];
                        if (pid == -1) continue;
                        if (room->disconnectPending[slot]) continue;
                        if (now - room->lastHeartbeatUs[slot] > HEARTBEAT_MS * 1000LL) {
                            cout << "[服务端] 玩家 " << room->playerNames[slot]
                                 << " 心跳超时" << endl;
                            room->disconnectPending[slot] = true;
                        }
                    }
                }
            }

            // Clean up empty rooms
            auto it = remove_if(rooms.begin(), rooms.end(),
                [](Room* r) {
                    bool empty = r->state == RoomState::WAITING &&
                                 r->playerIDs[0] == -1 &&
                                 r->playerIDs[1] == -1;
                    if (empty) delete r;
                    return empty;
                });
            rooms.erase(it, rooms.end());
        }

        int64_t elapsedUs = Clock::Now() - frameStart;
        int64_t sleepUs = TICK_INTERVAL * 1000LL - elapsedUs;
        if (sleepUs > 0) {
            this_thread::sleep_for(chrono::microseconds(sleepUs));
        }
    }
}
