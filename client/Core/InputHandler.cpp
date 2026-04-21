
// InputHandler.cpp
#include "InputHandler.h"

InputHandler::InputHandler() {
    reset();
}

void InputHandler::reset() {
    m_curInput = { false, false, false, false, false, false };
}

void InputHandler::setKeyDown(int key) {
    switch (key) {
        case KeyCode::W:     m_curInput.w = true;  break;
        case KeyCode::A:     m_curInput.a = true;  break;
        case KeyCode::S:     m_curInput.s = true;  break;
        case KeyCode::D:     m_curInput.d = true;  break;
        case KeyCode::SPACE: m_curInput.space = true; break;
        case KeyCode::F:     m_curInput.f = true;   break;
    }
}

void InputHandler::setKeyUp(int key) {
    switch (key) {
        case KeyCode::W:     m_curInput.w = false;  break;
        case KeyCode::A:     m_curInput.a = false;  break;
        case KeyCode::S:     m_curInput.s = false;  break;
        case KeyCode::D:     m_curInput.d = false;  break;
        case KeyCode::SPACE: m_curInput.space = false; break;
        case KeyCode::F:     m_curInput.f = false;   break;
    }
}

void InputHandler::update() {
    // 目前是直接查询状态，后续会加入平滑处理
    // （例如：按住W 10帧，而不是只记录第1帧）
}
