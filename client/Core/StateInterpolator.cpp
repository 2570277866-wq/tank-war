#include "StateInterpolator.h"
#include <cmath>

StateInterpolator::StateInterpolator()
    : m_prev{{0, 0}, 0}
    , m_target{{0, 0}, 0}
    , m_alpha(1.0f)
    , m_interpTime(0.05f)
{
}

void StateInterpolator::setTarget(const InterpState& target) {
    m_prev = getCurrent();
    m_target = target;
    m_alpha = 0.0f;
}

InterpState StateInterpolator::getCurrent() const {
    InterpState result;
    result.pos.x = m_prev.pos.x + (m_target.pos.x - m_prev.pos.x) * m_alpha;
    result.pos.y = m_prev.pos.y + (m_target.pos.y - m_prev.pos.y) * m_alpha;

    float da = m_target.angle - m_prev.angle;
    while (da > 3.14159265f) da -= 2.0f * 3.14159265f;
    while (da < -3.14159265f) da += 2.0f * 3.14159265f;
    result.angle = m_prev.angle + da * m_alpha;

    return result;
}

void StateInterpolator::update(float dt) {
    if (m_alpha < 1.0f) {
        m_alpha += dt / m_interpTime;
        if (m_alpha > 1.0f) m_alpha = 1.0f;
    }
}

void StateInterpolator::snap(const InterpState& state) {
    m_prev = state;
    m_target = state;
    m_alpha = 1.0f;
}
