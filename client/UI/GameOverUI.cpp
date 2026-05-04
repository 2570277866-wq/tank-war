#include "GameOverUI.h"
#include <Windows.h>

void GameOverUI::show(const GameOverInfo& info) {
    while (true) {
        cleardevice();
        setbkcolor(RGB(10, 10, 20));
        cleardevice();

        settextstyle(48, 0, L"黑体");
        if (info.playerWin) {
            settextcolor(RGB(50, 255, 50));
            outtextxy(300, 100, L"胜  利!");
        } else {
            settextcolor(RGB(255, 50, 50));
            outtextxy(300, 100, L"败  北!");
        }

        settextstyle(22, 0, L"黑体");
        settextcolor(RGB(200, 200, 200));

        wchar_t line1[64], line2[64], line3[64];
        swprintf_s(line1, L"击杀数: %d", info.playerKills);
        swprintf_s(line2, L"总伤害: %d", info.playerDamage);
        swprintf_s(line3, L"对局时长: %.0fs", info.duration);

        outtextxy(310, 220, line1);
        outtextxy(310, 260, line2);
        outtextxy(310, 300, line3);

        setfillcolor(RGB(50, 50, 150));
        setlinecolor(RGB(200, 200, 255));
        fillroundrect(300, 400, 500, 445, 8, 8);
        settextcolor(WHITE);
        settextstyle(20, 0, L"黑体");
        outtextxy(345, 412, L"返回主菜单");

        FlushBatchDraw();

        MOUSEMSG m = {};
        bool hasMsg = mousemsg();
        int mx = 0, my = 0;
        if (hasMsg) { m = getmousemsg(); mx = m.x; my = m.y; }

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN) {
            if (mx >= 300 && mx <= 500 && my >= 400 && my <= 445) return;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return;

        Sleep(16);
    }
}
