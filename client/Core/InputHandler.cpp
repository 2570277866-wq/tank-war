#include "InputHandler.h"

InputHandler::InputHandler() {
    reset();
}

void InputHandler::reset() {
    m_curInput = { false, false, false, false, false, false };
}

void InputHandler::update() {
    m_curInput.w     = (GetAsyncKeyState('W')     & 0x8000) != 0;
    m_curInput.a     = (GetAsyncKeyState('A')     & 0x8000) != 0;
    m_curInput.s     = (GetAsyncKeyState('S')     & 0x8000) != 0;
    m_curInput.d     = (GetAsyncKeyState('D')     & 0x8000) != 0;
    m_curInput.space = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    m_curInput.f     = (GetAsyncKeyState('F')     & 0x8000) != 0;
}
