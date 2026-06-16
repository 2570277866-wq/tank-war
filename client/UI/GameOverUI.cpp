#include "GameOverUI.h"
#include "TextHelper.h"
#include <Windows.h>

GameOverResult GameOverUI::show(const GameOverInfo& info) {
    // 清除上一局残留的鼠标消息，避免误触
    FlushMouseMsgBuffer();

    while (true) {
        cleardevice();
        setbkcolor(RGB(10, 10, 20));
        cleardevice();
        setfillstyle(BS_SOLID);  // 确保填充样式正常

        settextstyle(48, 0, "黑体");
        if (info.playerWin) {
            settextcolor(RGB(50, 255, 50));
            outtextxy_u8(300, 100, "胜  利!");
        } else {
            settextcolor(RGB(255, 50, 50));
            outtextxy_u8(300, 100, "败  北!");
        }

        settextstyle(22, 0, "黑体");
        settextcolor(RGB(200, 200, 200));

        char line1[64], line2[64], line3[64];
        snprintf(line1, sizeof(line1), "击杀数: %d", info.playerKills);
        snprintf(line2, sizeof(line2), "总伤害: %d", info.playerDamage);
        snprintf(line3, sizeof(line3), "对局时长: %.0fs", info.duration);

        outtextxy_u8(310, 220, line1);
        outtextxy_u8(310, 260, line2);
        outtextxy_u8(310, 300, line3);

        // 获取鼠标位置（只获取一次，共用）
        MOUSEMSG m = {};
        bool hasMsg = MouseHit();
        int mx = 0, my = 0;
        if (hasMsg) { m = GetMouseMsg(); mx = m.x; my = m.y; }

        // "再来一局" 按钮
        bool hoverAgain = (mx >= 220 && mx <= 420 && my >= 400 && my <= 445);
        setfillcolor(hoverAgain ? RGB(80, 160, 80) : RGB(40, 120, 40));
        setlinecolor(RGB(150, 255, 150));
        fillroundrect(220, 400, 420, 445, 8, 8);
        settextcolor(WHITE);
        settextstyle(20, 0, "黑体");
        outtextxy_u8(275, 412, "再来一局");

        // "返回主菜单" 按钮
        bool hoverMenu = (mx >= 460 && mx <= 660 && my >= 400 && my <= 445);
        setfillcolor(hoverMenu ? RGB(80, 80, 200) : RGB(50, 50, 150));
        setlinecolor(RGB(200, 200, 255));
        fillroundrect(460, 400, 660, 445, 8, 8);
        settextcolor(WHITE);
        settextstyle(20, 0, "黑体");
        outtextxy_u8(495, 412, "返回主菜单");

        FlushBatchDraw();

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN) {
            if (mx >= 220 && mx <= 420 && my >= 400 && my <= 445)
                return GameOverResult::PLAY_AGAIN;
            if (mx >= 460 && mx <= 660 && my >= 400 && my <= 445)
                return GameOverResult::MAIN_MENU;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            return GameOverResult::MAIN_MENU;

        Sleep(16);
    }
}
