#pragma once

#include <graphics.h>

enum class WaitResult {
    MATCHED,
    CANCEL
};

class WaitUI {
public:
    static WaitResult show(const wchar_t* statusText = L"等待对手加入...");
};
