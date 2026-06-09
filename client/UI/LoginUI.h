#pragma once

#include <graphics.h>
#include "Config.h"
#include <string>

enum class LoginResult {
    LOGIN_OK,
    REGISTER_OK,
    BACK
};

struct LoginInfo {
    std::string username;
    std::string password;
    std::string serverIP;
};

class LoginUI {
public:
    static LoginResult show(LoginInfo& outInfo);

private:
    static void drawInputBox(int x, int y, int w, int h,
                             const char* label,
                             const std::string& text,
                             bool focused, bool isPassword);
    static void drawButton(int x, int y, int w, int h,
                           const char* text, bool hovered);
    static bool isMouseOver(int mx, int my, int x, int y, int w, int h);
};
