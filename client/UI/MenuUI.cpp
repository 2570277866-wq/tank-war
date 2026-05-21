// MenuUI.cpp
#include "MenuUI.h"
#include <cstring>

// ===== 工具函数 =====
bool MenuUI::isMouseOver(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx <= x+w && my >= y && my <= y+h;
}

void MenuUI::drawButton(int x, int y, int w, int h,
                        const wchar_t* text, bool hovered) {
    setfillcolor(hovered ? RGB(80,80,200) : RGB(50,50,150));
    setlinecolor(RGB(200,200,255));
    fillroundrect(x, y, x+w, y+h, 10, 10);

    settextcolor(WHITE);
    settextstyle(24, 0, L"黑体");
    int tw = textwidth(text);
    int th = textheight(text);
    outtextxy(x + (w-tw)/2, y + (h-th)/2, text);
}

// ===== 主菜单 =====
MenuResult MenuUI::showMain() {
    int W = MapConfig::WIDTH, H = MapConfig::HEIGHT;
    int btnW = 200, btnH = 50;
    int btnX = (W - btnW) / 2;

    while (true) {
        MOUSEMSG m = {};
        bool hasMsg = mousemsg();

        // 获取鼠标位置
        int mx = 0, my = 0;
        if (hasMsg) {
            m = getmousemsg();
            mx = m.x; my = m.y;
        }

        // ===== 绘制 =====
        cleardevice();

        // 背景
        setbkcolor(RGB(20, 20, 40));
        cleardevice();

        // 标题
        settextcolor(RGB(255, 220, 50));
        settextstyle(48, 0, L"黑体");
        outtextxy(W / 2 - 120, 120, L"双人联机坦克大战");

        // 按钮
        bool hoverStart = isMouseOver(mx, my, btnX, 280, btnW, btnH);
        bool hoverExit  = isMouseOver(mx, my, btnX, 370, btnW, btnH);
        drawButton(btnX, 280, btnW, btnH, L"开始游戏", hoverStart);
        drawButton(btnX, 370, btnW, btnH, L"退  出",   hoverExit);

        FlushBatchDraw();

        // ===== 事件 =====
        if (hasMsg && m.uMsg == WM_LBUTTONDOWN) {
            if (hoverStart) return MenuResult::START_GAME;
            if (hoverExit)  return MenuResult::EXIT;
        }

        // ESC 退出
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            return MenuResult::EXIT;

        Sleep(16);
    }
}

// ===== 选坦克界面 =====
SelectResult MenuUI::showTankSelect() {
    int W = MapConfig::WIDTH, H = MapConfig::HEIGHT;

    struct TankOption {
        TankType type;
        const wchar_t* name;
        const wchar_t* desc;
        COLORREF color;
    };

    TankOption opts[3] = {
        { TankType::HEAVY, L"重型坦克", L"HP:200  速度:慢  伤害:大\n技能:铁壁护盾(3s免伤)", RGB(200,100,50) },
        { TankType::LIGHT, L"轻型坦克", L"HP:150  速度:中  伤害:中\n技能:涡轮冲刺(2s双倍速)", RGB(50,180,50) },
        { TankType::SCOUT, L"侦察坦克", L"HP:100  速度:快  伤害:小\n技能:弹幕散射(3颗扇形弹)", RGB(50,150,220) },
    };

    int selected = 1; // 默认选轻型

    while (true) {
        MOUSEMSG m = {};
        bool hasMsg = mousemsg();
        int mx = 0, my = 0;
        if (hasMsg) { m = getmousemsg(); mx = m.x; my = m.y; }

        cleardevice();
        setbkcolor(RGB(20, 20, 40));
        cleardevice();

        // 标题
        settextcolor(RGB(255,220,50));
        settextstyle(32, 0, L"黑体");
        outtextxy(270, 40, L"选择你的坦克");

        // 三个坦克卡片
        for (int i = 0; i < 3; i++) {
            int cx = 80 + i * 230;
            int cy = 120;
            int cw = 200, ch = 300;
            bool hover = isMouseOver(mx, my, cx, cy, cw, ch);
            bool isSel = (selected == i);

            // 卡片背景
            setfillcolor(isSel ? RGB(60,60,120) : (hover ? RGB(40,40,90) : RGB(30,30,60)));
            setlinecolor(isSel ? opts[i].color : RGB(80,80,80));
            setlinestyle(PS_SOLID, isSel ? 3 : 1);
            fillroundrect(cx, cy, cx+cw, cy+ch, 12, 12);

            // 坦克图标（简单画个矩形代替）
            setfillcolor(opts[i].color);
            fillrectangle(cx+60, cy+30, cx+140, cy+100);  // 车身
            fillrectangle(cx+80, cy+10, cx+120, cy+50);   // 炮塔
            fillrectangle(cx+108,cy+20, cx+128, cy+35);   // 炮管

            // 名字
            settextcolor(opts[i].color);
            settextstyle(22, 0, L"黑体");
            int tw = textwidth(opts[i].name);
            outtextxy(cx + (cw-tw)/2, cy+115, opts[i].name);

            // 描述
            settextcolor(RGB(200,200,200));
            settextstyle(16, 0, L"宋体");
            outtextxy(cx+10, cy+150, opts[i].desc);

            if (hover && hasMsg && m.uMsg == WM_LBUTTONDOWN)
                selected = i;
        }

        // 确认按钮
        bool hoverOK = isMouseOver(mx, my, 300, 470, 200, 50);
        drawButton(300, 470, 200, 50, L"确认选择", hoverOK);

        // 提示
        settextcolor(RGB(150,150,150));
        settextstyle(16, 0, L"宋体");
        outtextxy(280, 540, L"ESC 返回主菜单");

        FlushBatchDraw();

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN && hoverOK)
            return { opts[selected].type, true };

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            return { TankType::LIGHT, false };

        Sleep(16);
    }
}
