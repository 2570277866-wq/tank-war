#pragma once

#include <graphics.h>
#include <string>

enum class LoginResult {
    LOGIN_OK,
    REGISTER_OK,
    BACK
};

struct LoginInfo {
    std::wstring username;
    std::wstring password;
};

class LoginUI {
public:
    static LoginResult show(LoginInfo& outInfo);

private:
    static void drawInputBox(int x, int y, int w, int h,
                             const wchar_t* label,
                             const std::wstring& text,
                             bool focused, bool isPassword);
    static void drawButton(int x, int y, int w, int h,
                           const wchar_t* text, bool hovered);
    static bool isMouseOver(int mx, int my, int x, int y, int w, int h);
};
