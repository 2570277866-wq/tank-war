#include <WinSock2.h>
#include <graphics.h>
#include "Core/GameLoop.h"
#include "TextHelper.h"
#include "UI/MenuUI.h"
#include "UI/LoginUI.h"
#include "UI/WaitUI.h"
#include "UI/GameOverUI.h"
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

    // 缓存的登录凭据（一次登录，多局复用）
    std::string cachedUsername;
    std::string cachedPassword;
    std::string cachedServerIP = "127.0.0.1";
    bool hasCachedLogin = false;

    // 读取上次使用的服务器地址
    {
        std::ifstream f("server_ip.cfg");
        if (f.is_open()) {
            std::string savedIP;
            std::getline(f, savedIP);
            if (!savedIP.empty()) cachedServerIP = savedIP;
        }
    }

    // ===== 外层循环：主菜单 =====
    while (true) {
        MenuResult mr = MenuUI::showMain();
        if (mr == MenuResult::EXIT) break;

        // 排行榜：单独连接服务器获取数据
        if (mr == MenuResult::LEADERBOARD) {
            MenuUI::showLeaderboard(cachedServerIP.c_str());
            FlushMouseMsgBuffer();  // 清空残留鼠标消息
            continue;
        }

        // ===== 登录（仅首次或从主菜单返回时） =====
        if (!hasCachedLogin) {
            LoginInfo loginInfo;
            loginInfo.serverIP = cachedServerIP;
            loginInfo.username = cachedUsername;
            loginInfo.password = cachedPassword;

            LoginResult lr = LoginUI::show(loginInfo);
            if (lr == LoginResult::BACK) {
                FlushMouseMsgBuffer();
                continue;
            }

            // 保存本次使用的服务器地址
            {
                std::ofstream f("server_ip.cfg", std::ios::trunc);
                std::string ip(loginInfo.serverIP.begin(), loginInfo.serverIP.end());
                f << ip;
            }

            cachedUsername = loginInfo.username;
            cachedPassword = loginInfo.password;
            cachedServerIP = loginInfo.serverIP;
            hasCachedLogin = true;

            FlushMouseMsgBuffer();

            // ===== 连接服务端（带错误反馈和重试） =====
            bool connected = false;
            while (!connected) {
                // 显示 "正在连接..." 提示
                {
                    cleardevice();
                    setbkcolor(RGB(20, 20, 40));
                    cleardevice();
                    settextcolor(RGB(255, 220, 50));
                    settextstyle(28, 0, "黑体");
                    outtextxy_u8(350, 320, "正在连接服务器...");
                    settextcolor(RGB(150, 150, 200));
                    settextstyle(16, 0, "宋体");
                    std::string hint = "目标: " + cachedServerIP + ":" + std::to_string(SERVER_PORT);
                    outtextxy_u8(400, 380, hint.c_str());
                    FlushBatchDraw();
                }

                if (netClient.connect(cachedServerIP.c_str(), SERVER_PORT)) {
                    connected = true;
                    break;
                }

                // 连接失败：显示错误对话框，询问重试或返回
                bool retry = MenuUI::showMessage(
                    "连接服务器失败",
                    "请检查：\n"
                    "  1. 服务端是否已启动\n"
                    "  2. IP 地址是否正确\n"
                    "  3. 服务端防火墙是否放行端口 9527",
                    false);  // 重试 / 返回
                if (!retry) {
                    hasCachedLogin = false;
                    FlushMouseMsgBuffer();
                    break;  // 跳出 connect 循环 → 回到 login 选择
                }
                // retry → 再次尝试 connect
            }

            if (!connected) {
                // 用户选择了返回，回到登录界面
                // hasCachedLogin 已经 = false
                continue;  // 回到外层主菜单循环
            }

            // 发送登录或注册
            {
                char buf[256];
                uint16_t len;
                MsgCodec::encodeLogin(cachedUsername, cachedPassword, buf, len);

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

                auto authStart = std::chrono::high_resolution_clock::now();
                const float AUTH_TIMEOUT_SEC = 10.0f;  // 10 秒超时

                while (!authDone) {
                    netClient.poll();

                    auto now = std::chrono::high_resolution_clock::now();
                    float elapsed = std::chrono::duration<float>(now - authStart).count();
                    if (elapsed > AUTH_TIMEOUT_SEC) break;

                    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
                    Sleep(10);
                }

                if (!authDone || !authOK) {
                    netClient.disconnect();
                    hasCachedLogin = false;

                    const char* errTitle = nullptr;
                    const char* errSub   = nullptr;
                    if (!authDone) {
                        errTitle = "登录超时";
                        errSub   = "服务器无响应，请检查网络连接";
                    } else {
                        errTitle = "登录失败";
                        errSub   = "用户名或密码错误，请重试";
                    }
                    MenuUI::showMessage(errTitle, errSub);
                    FlushMouseMsgBuffer();
                    continue;
                }
            }
        }

        // ===== 内层循环：游戏大厅（保持登录，可连续多局） =====
        while (true) {
            FlushMouseMsgBuffer();

            // 选择坦克（返回 false 表示取消，回主菜单）
            SelectResult sr = MenuUI::showTankSelect();
            if (!sr.confirmed) {
                // 用户取消，离开房间并断开连接
                netClient.sendMsg(MsgID::C2S_LEAVE_ROOM);
                netClient.disconnect();
                hasCachedLogin = false;
                FlushMouseMsgBuffer();
                break;  // 回到主菜单
            }

            FlushMouseMsgBuffer();

            // 发送加入房间
            {
                char buf[256];
                uint16_t len;
                MsgCodec::encodeJoinRoom(cachedUsername, sr.type, buf, len);
                netClient.sendMsg(MsgID::C2S_JOIN_ROOM, buf, len);
            }

            // 等待匹配
            MatchResultData matchData{};
            std::atomic<bool> matched{false};

            // 清除旧的游戏回调，设置匹配回调
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
                FlushMouseMsgBuffer();
                continue;  // 回到坦克选择
            }

            FlushMouseMsgBuffer();

            // ===== 启动游戏 =====
            GameLoop game;
            game.setLocalTankType(sr.type);
            game.setNetClient(&netClient);
            game.initFromMatch(matchData, matchData.playerIDs[0]);
            game.start();

            // 设置游戏消息回调
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

            // 游戏主循环
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

            // 清除游戏回调（避免 GameLoop 销毁后回调访问野指针）
            netClient.setOnMsgCallback(nullptr);
            // 排空消息队列中的残留消息（如 S2C_GAME_OVER 等）
            netClient.poll();

            // 离开房间（服务端清理房间状态）
            netClient.sendMsg(MsgID::C2S_LEAVE_ROOM);

            // 显示结算界面
            {
                GameOverInfo info;
                info.playerWin    = game.isLocalWinner();
                info.playerKills  = game.getKills();
                info.playerDamage = game.getTotalDamage();
                info.duration     = game.getGameTime();
                GameOverResult result = GameOverUI::show(info);

                if (result == GameOverResult::MAIN_MENU) {
                    netClient.disconnect();
                    hasCachedLogin = false;
                    FlushMouseMsgBuffer();
                    break;  // 回到主菜单
                }
                // PLAY_AGAIN：继续内层循环（回到坦克选择）
            }
        }
    }

    EndBatchDraw();
    closegraph();
    return 0;
}
