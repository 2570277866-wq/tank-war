#pragma once

#include "../../Common/Protocol.h"
#include <graphics.h>
#include <vector>

struct DeathAnim {
    Vec2    pos;
    float   timer;
    float   maxTime;
    bool    active = false;
};

class DeathAnimator {
public:
    void trigger(Vec2 pos);
    void update(float dt);
    void draw();
    bool isActive() const;

private:
    std::vector<DeathAnim> m_anims;
};
