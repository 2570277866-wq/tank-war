#include <graphics.h>
#include "Core/GameLoop.h"
#include "UI/MenuUI.h"
#include "UI/LoginUI.h"
#include "Net/NetClient.h"
#include "Net/MsgCodec.h"

int main() {
    initgraph(800, 600, SHOWCONSOLE);
    setbkcolor(RGB(20, 20, 40));
    cleardevice();
    BeginBatchDraw();

    NetClient netClient;

    while (true) {
        MenuResult mr = MenuUI::showMain();
        if (mr == MenuResult::EXIT) break;

        LoginInfo loginInfo;
        LoginResult lr = LoginUI::show(loginInfo);
        if (lr == LoginResult::BACK) continue;

        SelectResult sr = MenuUI::showTankSelect();
        if (!sr.confirmed) continue;

        netClient.connect("127.0.0.1", SERVER_PORT);

        if (netClient.isConnected()) {
            char buf[256];
            uint16_t len;

            if (lr == LoginResult::LOGIN_OK) {
                MsgCodec::encodeLogin(
                    std::string(loginInfo.username.begin(), loginInfo.username.end()),
                    std::string(loginInfo.password.begin(), loginInfo.password.end()),
                    buf, len);
                netClient.sendMsg(MsgID::C2S_LOGIN, buf, len);
            } else {
                MsgCodec::encodeLogin(
                    std::string(loginInfo.username.begin(), loginInfo.username.end()),
                    std::string(loginInfo.password.begin(), loginInfo.password.end()),
                    buf, len);
                netClient.sendMsg(MsgID::C2S_REGISTER, buf, len);
            }

            MsgCodec::encodeJoinRoom(
                std::string(loginInfo.username.begin(), loginInfo.username.end()),
                sr.type, buf, len);
            netClient.sendMsg(MsgID::C2S_JOIN_ROOM, buf, len);
        }

        GameLoop game;
        game.setLocalTankType(sr.type);
        game.setNetClient(&netClient);
        game.start();

        while (game.isRunning() && !game.isGameOver()) {
            game.update(0.016f);
            game.draw();
            netClient.poll();
            Sleep(16);
        }

        game.draw();

        while (true) {
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
            Sleep(50);
        }

        netClient.disconnect();
    }

    EndBatchDraw();
    closegraph();
    return 0;
}
