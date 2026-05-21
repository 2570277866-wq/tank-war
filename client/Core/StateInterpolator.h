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

private:
    InterpState m_prev;
    InterpState m_target;
    float       m_alpha;
    float       m_interpTime;
};
