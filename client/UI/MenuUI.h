// MenuUI.h
#pragma once
#include <graphics.h>
#include "../../Common/Protocol.h"

enum class MenuResult { START_GAME, EXIT };

struct SelectResult {
    TankType type;
    bool confirmed;
};

class MenuUI {
public:
    static MenuResult  showMain();        // 主菜单
    static SelectResult showTankSelect(); // 选坦克界面
private:
    static void drawButton(int x, int y, int w, int h,
                           const wchar_t* text, bool hovered);
    static bool isMouseOver(int mx, int my, int x, int y, int w, int h);
};
