#include "LoginUI.h"
#include <Windows.h>

bool LoginUI::isMouseOver(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

void LoginUI::drawButton(int x, int y, int w, int h,
                         const char* text, bool hovered) {
    setfillcolor(hovered ? RGB(80, 80, 200) : RGB(50, 50, 150));
    setlinecolor(RGB(200, 200, 255));
    setlinestyle(PS_SOLID, 1);
    fillroundrect(x, y, x + w, y + h, 8, 8);

    settextcolor(WHITE);
    settextstyle(20, 0, "黑体");
    int tw = textwidth(text);
    int th = textheight(text);
    outtextxy(x + (w - tw) / 2, y + (h - th) / 2, text);
}

void LoginUI::drawInputBox(int x, int y, int w, int h,
                           const char* label,
                           const std::string& text,
                           bool focused, bool isPassword) {
    settextcolor(RGB(200, 200, 200));
    settextstyle(18, 0, "黑体");
    outtextxy(x, y - 24, label);

    setfillcolor(RGB(30, 30, 60));
    setlinecolor(focused ? RGB(100, 150, 255) : RGB(80, 80, 100));
    setlinestyle(PS_SOLID, focused ? 2 : 1);
    fillroundrect(x, y, x + w, y + h, 6, 6);

    std::string display;
    if (isPassword) {
        display.append(text.length(), '*');
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
            display += "|";
        }
    }

    settextcolor(WHITE);
    settextstyle(18, 0, "宋体");
    outtextxy(x + 10, y + (h - textheight("A")) / 2, display.c_str());
}

LoginResult LoginUI::show(LoginInfo& outInfo) {
    // 如果传入了默认地址就用它，否则用 127.0.0.1
    std::string serverAddr = outInfo.serverIP.empty()
        ? "127.0.0.1" : outInfo.serverIP;
    std::string username;
    std::string password;
    int focus = 0;
    const int MAX_LEN = 20;

    int boxX = (MapConfig::WIDTH - 300) / 2, boxW = 300, boxH = 36;
    int serverY = 160;
    int userY   = 240;
    int passY   = 320;

    int btnW = 120, btnH = 42;
    int loginX = boxX, registerX = boxX + 180, btnY = 400;

    int heldChar = 0;        // 上一帧按下的字符键（消抖）

    while (true) {
        MOUSEMSG m = {};
        bool hasMsg = MouseHit();
        int mx = 0, my = 0;
        if (hasMsg) { m = GetMouseMsg(); mx = m.x; my = m.y; }

        cleardevice();
        setbkcolor(RGB(20, 20, 40));
        cleardevice();

        settextcolor(RGB(255, 220, 50));
        settextstyle(36, 0, "黑体");
        outtextxy(280, 100, "登  录");

        drawInputBox(boxX, serverY, boxW, boxH,
                     "服务器地址", serverAddr, focus == 0, false);
        drawInputBox(boxX, userY, boxW, boxH,
                     "用户名", username, focus == 1, false);
        drawInputBox(boxX, passY, boxW, boxH,
                     "密  码", password, focus == 2, true);

        bool hoverLogin    = isMouseOver(mx, my, loginX, btnY, btnW, btnH);
        bool hoverRegister = isMouseOver(mx, my, registerX, btnY, btnW, btnH);
        drawButton(loginX, btnY, btnW, btnH, "登  录", hoverLogin);
        drawButton(registerX, btnY, btnW, btnH, "注  册", hoverRegister);

        settextcolor(RGB(150, 150, 150));
        settextstyle(14, 0, "宋体");
        outtextxy(260, 440, "Tab 切换输入框    Enter 提交登录");
        outtextxy(300, 465, "ESC 返回主菜单");

        FlushBatchDraw();

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN) {
            if (isMouseOver(mx, my, boxX, serverY, boxW, boxH)) focus = 0;
            if (isMouseOver(mx, my, boxX, userY, boxW, boxH)) focus = 1;
            if (isMouseOver(mx, my, boxX, passY, boxW, boxH)) focus = 2;

            if (hoverLogin && !username.empty() && !password.empty()) {
                outInfo.serverIP = serverAddr;
                outInfo.username = username;
                outInfo.password = password;
                return LoginResult::LOGIN_OK;
            }
            if (hoverRegister && !username.empty() && !password.empty()) {
                outInfo.serverIP = serverAddr;
                outInfo.username = username;
                outInfo.password = password;
                return LoginResult::REGISTER_OK;
            }
        }

        if (GetAsyncKeyState(VK_TAB) & 0x8000) {
            focus = (focus + 1) % 3;
            Sleep(150);
        }

        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            if (!username.empty() && !password.empty()) {
                outInfo.serverIP = serverAddr;
                outInfo.username = username;
                outInfo.password = password;
                return LoginResult::LOGIN_OK;
            }
            Sleep(150);
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            return LoginResult::BACK;
        }

        bool keyFound = false;
        for (int c = 0x20; c <= 0x7E; c++) {
            if (GetAsyncKeyState(c) & 0x8000) {
                if (c != heldChar) {   // 新按键（非重复）
                    std::string* target =
                        (focus == 0) ? &serverAddr :
                        (focus == 1) ? &username : &password;
                    if ((int)target->length() < MAX_LEN) {
                        target->push_back((char)c);
                    }
                    heldChar = c;
                }
                keyFound = true;
                break;
            }
        }

        // 服务端地址允许输入 '.' 和 ':'
        if (focus == 0 && !keyFound) {
            int extra[] = {VK_OEM_PERIOD, VK_OEM_1};
            char chars[] = {'.', ':'};
            for (int i = 0; i < 2; i++) {
                if (GetAsyncKeyState(extra[i]) & 0x8000) {
                    if (extra[i] != heldChar) {
                        if ((int)serverAddr.length() < MAX_LEN) {
                            serverAddr.push_back(chars[i]);
                        }
                        heldChar = extra[i];
                    }
                    keyFound = true;
                    break;
                }
            }
        }

        if (!keyFound) heldChar = 0;  // 所有键释放

        if (GetAsyncKeyState(VK_BACK) & 0x8000) {
            std::string* target =
                (focus == 0) ? &serverAddr :
                (focus == 1) ? &username : &password;
            if (!target->empty()) target->pop_back();
            Sleep(80);
        }

        Sleep(16);
    }
}
