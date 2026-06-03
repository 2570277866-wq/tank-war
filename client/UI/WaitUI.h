#pragma once

#include <graphics.h>
#include <atomic>

enum class WaitResult {
    MATCHED,
    CANCEL
};

class WaitUI {
public:
    static WaitResult show(const wchar_t* statusText, const std::atomic<bool>* matched = nullptr);
};
