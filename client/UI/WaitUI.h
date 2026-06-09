#pragma once

#include <graphics.h>
#include <atomic>
#include <functional>

enum class WaitResult {
    MATCHED,
    CANCEL
};

class WaitUI {
public:
    static WaitResult show(const char* statusText,
                           const std::atomic<bool>* matched = nullptr,
                           std::function<void()> pollFn = nullptr);
};
