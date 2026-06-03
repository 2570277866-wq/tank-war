#include "LoginUI.h"
#include <Windows.h>

bool LoginUI::isMouseOver(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

void LoginUI::drawButton(int x, int y, int w, int h,
                         const wchar_t* text, bool hovered) {
    setfillcolor(hovered ? RGB(80, 80, 200) : RGB(50, 50, 150));
    setlinecolor(RGB(200, 200, 255));
    setlinestyle(PS_SOLID, 1);
    fillroundrect(x, y, x + w, y + h, 8, 8);

    settextcolor(WHITE);
    settextstyle(20, 0, L"黑体");
    int tw = textwidth(text);
    int th = textheight(text);
    outtextxy(x + (w - tw) / 2, y + (h - th) / 2, text);
}

void LoginUI::drawInputBox(int x, int y, int w, int h,
                           const wchar_t* label,
                           const std::wstring& text,
                           bool focused, bool isPassword) {
    settextcolor(RGB(200, 200, 200));
    settextstyle(18, 0, L"黑体");
    outtextxy(x, y - 24, label);

    setfillcolor(RGB(30, 30, 60));
    setlinecolor(focused ? RGB(100, 150, 255) : RGB(80, 80, 100));
    setlinestyle(PS_SOLID, focused ? 2 : 1);
    fillroundrect(x, y, x + w, y + h, 6, 6);

    std::wstring display;
    if (isPassword) {
        display.append(text.length(), L'*');
    } else {
        display = text;
    }

    if (focused) {
        static DWORD lastTick = 0;
        static bool cursorShow = true;
        DWORD now = GetTickCount();
        if (now - lastTick > 500) {
            cursorShow = !cursorShow;
            lastTick = now;
        }
        if (cursorShow) {
            display += L"|";
        }
    }

    settextcolor(WHITE);
    settextstyle(18, 0, L"宋体");
    outtextxy(x + 10, y + (h - textheight(L"A")) / 2, display.c_str());
}

LoginResult LoginUI::show(LoginInfo& outInfo) {
    std::wstring username;
    std::wstring password;
    int focus = 0;
    const int MAX_LEN = 20;

    int boxX = (MapConfig::WIDTH - 300) / 2, boxW = 300, boxH = 36;
    int userY = 200;
    int passY = 290;

    int btnW = 120, btnH = 42;
    int loginX = boxX, registerX = boxX + 180, btnY = 370;

    while (true) {
        MOUSEMSG m = {};
        bool hasMsg = MouseHit();
        int mx = 0, my = 0;
        if (hasMsg) { m = GetMouseMsg(); mx = m.x; my = m.y; }

        cleardevice();
        setbkcolor(RGB(20, 20, 40));
        cleardevice();

        settextcolor(RGB(255, 220, 50));
        settextstyle(36, 0, L"黑体");
        outtextxy(280, 100, L"登  录");

        drawInputBox(boxX, userY, boxW, boxH,
                     L"用户名", username, focus == 0, false);
        drawInputBox(boxX, passY, boxW, boxH,
                     L"密  码", password, focus == 1, true);

        bool hoverLogin    = isMouseOver(mx, my, loginX, btnY, btnW, btnH);
        bool hoverRegister = isMouseOver(mx, my, registerX, btnY, btnW, btnH);
        drawButton(loginX, btnY, btnW, btnH, L"登  录", hoverLogin);
        drawButton(registerX, btnY, btnW, btnH, L"注  册", hoverRegister);

        settextcolor(RGB(150, 150, 150));
        settextstyle(14, 0, L"宋体");
        outtextxy(260, 440, L"Tab 切换输入框    Enter 提交登录");
        outtextxy(300, 465, L"ESC 返回主菜单");

        FlushBatchDraw();

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN) {
            if (isMouseOver(mx, my, boxX, userY, boxW, boxH)) focus = 0;
            if (isMouseOver(mx, my, boxX, passY, boxW, boxH)) focus = 1;

            if (hoverLogin && !username.empty() && !password.empty()) {
                outInfo.username = username;
                outInfo.password = password;
                return LoginResult::LOGIN_OK;
            }
            if (hoverRegister && !username.empty() && !password.empty()) {
                outInfo.username = username;
                outInfo.password = password;
                return LoginResult::REGISTER_OK;
            }
        }

        if (GetAsyncKeyState(VK_TAB) & 0x8000) {
            focus = 1 - focus;
            Sleep(150);
        }

        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            if (!username.empty() && !password.empty()) {
                outInfo.username = username;
                outInfo.password = password;
                return LoginResult::LOGIN_OK;
            }
            Sleep(150);
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            return LoginResult::BACK;
        }

        for (int c = 0x20; c <= 0x7E; c++) {
            if (GetAsyncKeyState(c) & 0x8000) {
                std::wstring* target = (focus == 0) ? &username : &password;
                if ((int)target->length() < MAX_LEN) {
                    target->push_back((wchar_t)c);
                }
                Sleep(80);
                break;
            }
        }

        if (GetAsyncKeyState(VK_BACK) & 0x8000) {
            std::wstring* target = (focus == 0) ? &username : &password;
            if (!target->empty()) target->pop_back();
            Sleep(80);
        }

        Sleep(16);
    }
}
