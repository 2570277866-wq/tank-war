#include "WaitUI.h"
#include <Windows.h>

WaitResult WaitUI::show(const wchar_t* statusText) {
    int dotCount = 0;
    DWORD lastTick = GetTickCount();

    while (true) {
        DWORD now = GetTickCount();
        if (now - lastTick > 500) {
            dotCount = (dotCount + 1) % 4;
            lastTick = now;
        }

        cleardevice();
        setbkcolor(RGB(20, 20, 40));
        cleardevice();

        settextcolor(RGB(255, 220, 50));
        settextstyle(32, 0, L"黑体");
        outtextxy(280, 200, L"匹配中");

        wchar_t dots[5] = {};
        for (int i = 0; i < dotCount; i++) dots[i] = L'.';
        settextcolor(RGB(200, 200, 200));
        settextstyle(24, 0, L"黑体");
        outtextxy(310, 270, dots);

        settextcolor(RGB(150, 150, 200));
        settextstyle(18, 0, L"宋体");
        outtextxy(250, 340, statusText);

        setfillcolor(RGB(60, 60, 60));
        setlinecolor(RGB(100, 100, 100));
        setlinestyle(PS_SOLID, 1);
        fillroundrect(300, 420, 500, 460, 8, 8);
        settextcolor(RGB(200, 200, 200));
        settextstyle(18, 0, L"黑体");
        outtextxy(355, 432, L"取消匹配");

        MOUSEMSG m = {};
        bool hasMsg = mousemsg();
        int mx = 0, my = 0;
        if (hasMsg) { m = getmousemsg(); mx = m.x; my = m.y; }

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN) {
            if (mx >= 300 && mx <= 500 && my >= 420 && my <= 460) {
                return WaitResult::CANCEL;
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            return WaitResult::CANCEL;
        }

        FlushBatchDraw();
        Sleep(16);
    }
}
