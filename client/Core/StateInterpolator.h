#pragma once

#include "Protocol.h"

struct InterpState {
    Vec2  pos;
    float angle;
};

class StateInterpolator {
public:
    StateInterpolator();

    void setTarget(const InterpState& target);
    InterpState getCurrent() const;
    void update(float dt);

    void snap(const InterpState& state);

    // 动态调整插值时间（配合实际快照间隔，避免固定 50ms 导致的突跳）
    void setInterpTime(float t) { m_interpTime = t; }

private:
    InterpState m_prev;
    InterpState m_target;
    float       m_alpha;
    float       m_interpTime;
};
