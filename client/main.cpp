#include <WinSock2.h>
#include <graphics.h>
#include "Core/GameLoop.h"
#include "UI/MenuUI.h"
#include "UI/LoginUI.h"
#include "UI/WaitUI.h"
#include "Net/NetClient.h"
#include "Net/MsgCodec.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <string>

int main() {
    initgraph(MapConfig::WIDTH, MapConfig::HEIGHT, SHOWCONSOLE);
    setbkcolor(RGB(20, 20, 40));
    cleardevice();
    BeginBatchDraw();

    NetClient netClient;

    while (true) {
        MenuResult mr = MenuUI::showMain();
        if (mr == MenuResult::EXIT) break;

        LoginInfo loginInfo;

        // 读取上次使用的服务器地址
        {
            std::ifstream f("server_ip.cfg");
            if (f.is_open()) {
                std::string savedIP;
                std::getline(f, savedIP);
                if (!savedIP.empty()) {
                    loginInfo.serverIP.assign(savedIP.begin(), savedIP.end());
                }
            }
        }

        LoginResult lr = LoginUI::show(loginInfo);
        if (lr == LoginResult::BACK) continue;

        // 保存本次使用的服务器地址
        {
            std::ofstream f("server_ip.cfg", std::ios::trunc);
            std::string ip(loginInfo.serverIP.begin(), loginInfo.serverIP.end());
            f << ip;
        }

        SelectResult sr = MenuUI::showTankSelect();
        if (!sr.confirmed) continue;

        // 连接服务端（使用登录界面填写的服务器地址）
        {
            std::string ip(loginInfo.serverIP.begin(), loginInfo.serverIP.end());
            if (!netClient.connect(ip.c_str(), SERVER_PORT))
                continue;
        }

        // 发送登录或注册
        {
            char buf[256];
            uint16_t len;
            MsgCodec::encodeLogin(
                std::string(loginInfo.username.begin(), loginInfo.username.end()),
                std::string(loginInfo.password.begin(), loginInfo.password.end()),
                buf, len);

            if (lr == LoginResult::LOGIN_OK)
                netClient.sendMsg(MsgID::C2S_LOGIN, buf, len);
            else
                netClient.sendMsg(MsgID::C2S_REGISTER, buf, len);
        }

        // 等待登录/注册的服务器回复
        {
            std::atomic<bool> authDone{false};
            std::atomic<bool> authOK{false};

            netClient.setOnMsgCallback([&](MsgID id, const char* data, uint16_t len) {
                if (id == MsgID::S2C_LOGIN_ACK || id == MsgID::S2C_REGISTER_ACK) {
                    ErrorCode code = MsgCodec::decodeLoginAck(data, len);
                    authOK = (code == ErrorCode::NONE);
                    authDone = true;
                }
            });

            while (!authDone) {
                netClient.poll();
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
                Sleep(10);
            }

            if (!authOK) {
                netClient.disconnect();
                continue;
            }
        }

        // 发送加入房间
        {
            char buf[256];
            uint16_t len;
            MsgCodec::encodeJoinRoom(
                std::string(loginInfo.username.begin(), loginInfo.username.end()),
                sr.type, buf, len);
            netClient.sendMsg(MsgID::C2S_JOIN_ROOM, buf, len);
        }

        // 等待匹配 - 服务端匹配成功时 matched 被设为 true
        MatchResultData matchData{};
        std::atomic<bool> matched{false};
        netClient.setOnMsgCallback([&](MsgID id, const char* data, uint16_t len) {
            if (id == MsgID::S2C_MATCH_RESULT) {
                matchData = MsgCodec::decodeMatchResult(data, len);
                matched = true;
            }
        });

        WaitResult wr = WaitUI::show("等待对手加入...", &matched,
            [&] { netClient.poll(); });
        if (wr == WaitResult::CANCEL || !matched) {
            netClient.sendMsg(MsgID::C2S_LEAVE_ROOM);
            netClient.disconnect();
            continue;
        }

        // 启动游戏（playerIDs[0] 是本地玩家 ID）
        GameLoop game;
        game.setLocalTankType(sr.type);
        game.setNetClient(&netClient);
        game.initFromMatch(matchData, matchData.playerIDs[0]);
        game.start();

        netClient.setOnMsgCallback([&game](MsgID id, const char* data, uint16_t len) {
            switch (id) {
                case MsgID::S2C_SNAPSHOT: {
                    Snapshot snap = MsgCodec::decodeSnapshot(data, len);
                    game.applySnapshot(snap);
                    break;
                }
                case MsgID::S2C_HIT: {
                    HitData hit = MsgCodec::decodeHitData(data, len);
                    game.onHitReceived(hit);
                    break;
                }
                case MsgID::S2C_GAME_OVER:
                    break;
                default:
                    break;
            }
        });

        auto prevTime = std::chrono::high_resolution_clock::now();
        while (game.isRunning() && !game.isGameOver()) {
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - prevTime).count();
            prevTime = now;

            // 防止帧间隔过大导致物理穿墙
            if (dt > 0.05f) dt = 0.05f;
            if (dt <= 0.0f) dt = 0.001f;

            game.update(dt);
            game.draw();
            netClient.poll();

            // 动态休眠，维持约 60fps
            auto after = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(after - now).count();
            int sleepMs = (int)((16.667f - elapsed * 1000.0f));
            if (sleepMs > 0 && sleepMs <= 30) Sleep(sleepMs);
        }

        game.draw();
        Sleep(1500);
        netClient.disconnect();
    }

    EndBatchDraw();
    closegraph();
    return 0;
}
