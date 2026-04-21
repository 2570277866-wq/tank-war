// InputHandler.h
#pragma once

#include "../../Common/Protocol.h"

// 输入来源（跨平台兼容）
enum class InputSource {
    KEYBOARD  // 键盘WASD/空格/F
};

class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    // 每帧调用，填充当前输入状态
    void update();

    // 查询当前输入状态
    InputState getState() const { return m_curInput; }

    // 设置输入来源（后续 SDL2/EasyX 平台层会调用这些）
    void setKeyDown(int key);
    void setKeyUp(int key);

    // 重置所有按键
    void reset();

private:
    InputState m_curInput;
};

// ===== 键码定义（平台无关）=====
namespace KeyCode {
    constexpr int W     = 0x01;
    constexpr int A     = 0x02;
    constexpr int S     = 0x03;
    constexpr int D     = 0x04;
    constexpr int SPACE = 0x10;
    constexpr int F     = 0x20;
}