#pragma once

// TextHelper.h
// EasyX 使用 ANSI (GBK) 版本的 GDI 文字函数，而项目源码是 UTF-8。
// 本头文件提供包装函数，在运行时将 UTF-8 转为系统 ANSI 编码，
// 解决 MSVC /utf-8 与 MinGW 无 -fexec-charset 时的中文乱码问题。

#include <graphics.h>
#include <string>
#include <windows.h>

// 将 UTF-8 字符串转为当前系统 ANSI 编码（中文 Windows 下为 GBK）
inline std::string utf8ToAnsi(const char* utf8) {
    if (!utf8 || !*utf8) return {};

    // UTF-8 -> UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (wlen <= 0) return utf8;
    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &wstr[0], wlen);

    // UTF-16 -> 系统 ANSI (CP_ACP: 中文 Windows = GBK/CP936)
    int alen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1,
                                   nullptr, 0, nullptr, nullptr);
    if (alen <= 0) return utf8;
    std::string ansi(alen, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1,
                        &ansi[0], alen, nullptr, nullptr);

    // 去掉末尾的 '\0'
    if (!ansi.empty() && ansi.back() == '\0')
        ansi.pop_back();
    return ansi;
}

// ===== 包装 EasyX 文字函数，自动做 UTF-8 -> ANSI 转换 =====

inline void outtextxy_u8(int x, int y, const char* utf8) {
    std::string ansi = utf8ToAnsi(utf8);
    outtextxy(x, y, ansi.c_str());
}

inline void outtextxy_u8(int x, int y, const std::string& utf8) {
    outtextxy_u8(x, y, utf8.c_str());
}

inline int textwidth_u8(const char* utf8) {
    std::string ansi = utf8ToAnsi(utf8);
    return textwidth(ansi.c_str());
}

inline int textheight_u8(const char* utf8) {
    std::string ansi = utf8ToAnsi(utf8);
    return textheight(ansi.c_str());
}
