#pragma once

#include "Protocol.h"
#include <Windows.h>

class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    void update();
    InputState getState() const { return m_curInput; }
    void reset();

private:
    InputState m_curInput;
};
