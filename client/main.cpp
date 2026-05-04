#include <graphics.h>
#include "Core/GameLoop.h"
#include "UI/MenuUI.h"
#include "UI/LoginUI.h"

int main() {
    initgraph(800, 600, SHOWCONSOLE);
    setbkcolor(RGB(20, 20, 40));
    cleardevice();
    BeginBatchDraw();

    while (true) {
        MenuResult mr = MenuUI::showMain();
        if (mr == MenuResult::EXIT) break;

        LoginInfo loginInfo;
        LoginResult lr = LoginUI::show(loginInfo);
        if (lr == LoginResult::BACK) continue;

        SelectResult sr = MenuUI::showTankSelect();
        if (!sr.confirmed) continue;

        GameLoop game;
        game.setLocalTankType(sr.type);
        game.start();

        while (game.isRunning() && !game.isGameOver()) {
            game.update(0.016f);
            game.draw();
            Sleep(16);
        }

        game.draw();

        while (true) {
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
            Sleep(50);
        }
    }

    EndBatchDraw();
    closegraph();
    return 0;
}
