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

    // 显示错误/提示对话框，阻塞直到用户点击或按任意键关闭
    // okOnly=true   → 仅显示 "确定" 按钮
    // okOnly=false  → 显示 "重试" + "返回" 按钮，返回 true=重试
    static bool         showMessage(const char* title, const char* subtitle,
                                    bool okOnly = true);

private:
    static void drawButton(int x, int y, int w, int h,
                           const char* text, bool hovered);
    static bool isMouseOver(int mx, int my, int x, int y, int w, int h);
};
