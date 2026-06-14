// MenuUI.h
#pragma once
#include <graphics.h>
#include "Protocol.h"
#include "Config.h"

enum class MenuResult { START_GAME, LEADERBOARD, EXIT };

struct SelectResult {
    TankType type;
    bool confirmed;
};

class MenuUI {
public:
    static MenuResult   showMain();          // 主菜单
    static SelectResult showTankSelect();    // 选坦克界面
    static void         showLeaderboard(const char* serverIP); // 排行榜
private:
    static void drawButton(int x, int y, int w, int h,
                           const char* text, bool hovered);
    static bool isMouseOver(int mx, int my, int x, int y, int w, int h);
};
