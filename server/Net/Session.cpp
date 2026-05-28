#include "Session.h"
#include "../Data/UserManager.h"
#include "../Data/Leaderboard.h"
#include "../Game/Room.h"
#include "../Core/Logger.h"
#include <iostream>
#include <cstring>

using namespace std;

Session::Session(SOCKET s) : sock(s), recvLen(0) {
    memset(recvBuf, 0, BUFFER_SIZE);
    lastHeartbeatUs = Clock::Now();
}

void Session::Send(MsgID id, const void* body, uint16_t bodyLen) {
    lock_guard<mutex> lock(sendMutex);

    MsgHeader header;
    header.id = id;
    header.bodyLen = bodyLen;

    int ret = send(sock, (const char*)&header, sizeof(header), 0);
    if (ret <= 0) return;

    if (body && bodyLen > 0)
        send(sock, (const char*)body, bodyLen, 0);
}

// ============ Message Handlers ============

static void HandleRegister(Session* session, const char* body, int /*bodyLen*/) {
    string account(body);
    string pwd(body + account.size() + 1);

    Logger::Get().Info("注册请求：账号=" + account);

    bool ok = Register(account, pwd);
    ErrorCode code = ok ? ErrorCode::NONE : ErrorCode::REGISTER_EXISTS;

    session->Send(MsgID::S2C_REGISTER_ACK, &code, sizeof(code));
    Logger::Get().Info(std::string("注册") + (ok ? "成功" : "失败（账号已存在）"));
}

static void HandleLogin(Session* session, const char* body, int /*bodyLen*/) {
    string account(body);
    string pwd(body + account.size() + 1);

    Logger::Get().Info("登录请求：账号=" + account);

    bool ok = Login(account, pwd);
    ErrorCode code = ok ? ErrorCode::NONE : ErrorCode::LOGIN_FAILED;

    if (ok) {
        session->userName = account;
        session->isLoggedIn = true;
    }

    session->Send(MsgID::S2C_LOGIN_ACK, &code, sizeof(code));
    Logger::Get().Info(std::string("登录") + (ok ? "成功" : "失败"));
}

void ProcessMsg(Session* session, MsgID msgId, const char* body, int bodyLen) {
    switch (msgId) {

    case MsgID::C2S_REGISTER:
        HandleRegister(session, body, bodyLen);
        break;

    case MsgID::C2S_LOGIN:
        HandleLogin(session, body, bodyLen);
        break;

    case MsgID::C2S_JOIN_ROOM:
        if (session->onJoinRoom)
            session->onJoinRoom(session->playerID);
        break;

    case MsgID::C2S_LEAVE_ROOM:
        if (session->onLeaveRoom)
            session->onLeaveRoom(session->playerID);
        break;

    case MsgID::C2S_SELECT_TANK:
        if (session->currentRoom && bodyLen >= (int)sizeof(SelectTankReq)) {
            SelectTankReq req;
            memcpy(&req, body, sizeof(req));
            session->currentRoom->SelectTank(session->playerID, req.type);
        }
        break;

    case MsgID::C2S_INPUT:
        if (session->currentRoom && bodyLen >= (int)sizeof(InputState)) {
            InputState input;
            memcpy(&input, body, sizeof(InputState));
            session->currentRoom->OnPlayerInput(session->playerID, input);
        }
        break;

    case MsgID::C2S_SHOOT:
        if (session->currentRoom)
            session->currentRoom->OnPlayerShoot(session->playerID);
        break;

    case MsgID::C2S_USE_SKILL:
        if (session->currentRoom)
            session->currentRoom->OnPlayerSkill(session->playerID);
        break;

    case MsgID::C2S_HEARTBEAT:
        session->lastHeartbeatUs = Clock::Now();
        session->Send(MsgID::S2C_HEARTBEAT, nullptr, 0);
        if (session->currentRoom)
            session->currentRoom->OnHeartbeat(session->playerID);
        break;

    case MsgID::C2S_GET_RANK:
        {
            RankListData rank = GetTopPlayers(10);
            session->Send(MsgID::S2C_RANK_LIST, &rank, sizeof(rank));
        }
        break;

    case MsgID::C2S_RECONNECT:
        if (bodyLen >= (int)sizeof(ReconnectReq)) {
            ReconnectReq req;
            memcpy(&req, body, sizeof(req));
            req.username[31] = '\0'; // ensure null-terminated
            if (session->onTryReconnect)
                session->onTryReconnect(session->playerID, string(req.username));
        }
        break;

    default:
        Logger::Get().Warn("未知消息类型 msgId=0x" + std::to_string((uint16_t)msgId));
        break;
    }
}

// ============ Recv Thread ============

void RecvThread(Session* session) {
    char tempBuf[BUFFER_SIZE];

    while (true) {
        int ret = recv(session->sock, tempBuf, BUFFER_SIZE, 0);
        if (ret <= 0)
            break;

        if (session->recvLen + ret > BUFFER_SIZE) {
            Logger::Get().Error("缓冲区溢出，强制断开连接");
            break;
        }

        memcpy(session->recvBuf + session->recvLen, tempBuf, ret);
        session->recvLen += ret;

        while (session->recvLen >= (int)sizeof(MsgHeader)) {
            MsgHeader header;
            memcpy(&header, session->recvBuf, sizeof(MsgHeader));

            unsigned int totalLen = sizeof(MsgHeader) + header.bodyLen;
            if (session->recvLen < (int)totalLen)
                break;

            ProcessMsg(session, header.id,
                       session->recvBuf + sizeof(MsgHeader),
                       header.bodyLen);

            session->recvLen -= (int)totalLen;
            memmove(session->recvBuf, session->recvBuf + totalLen, session->recvLen);
        }
    }

    // Notify room of disconnect via atomic flag
    if (session->currentRoom) {
        int slot = session->currentRoom->GetSlot(session->playerID);
        if (slot >= 0) {
            session->currentRoom->disconnectPending[slot] = true;
        }
    }

    if (session->onDisconnect)
        session->onDisconnect();

    closesocket(session->sock);
    Logger::Get().Info("客户端断开连接");
    delete session;
}

void Session::StartRecv() {
    thread(RecvThread, this).detach();
}
