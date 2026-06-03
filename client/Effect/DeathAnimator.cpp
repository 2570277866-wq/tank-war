#include "DeathAnimator.h"
#include <cmath>
#include <algorithm>

void DeathAnimator::trigger(Vec2 pos) {
    DeathAnim anim;
    anim.pos = pos;
    anim.timer = 1.5f;
    anim.maxTime = 1.5f;
    anim.active = true;
    m_anims.push_back(anim);
}

void DeathAnimator::update(float dt) {
    for (auto& a : m_anims) {
        if (!a.active) continue;
        a.timer -= dt;
        if (a.timer <= 0) a.active = false;
    }
    m_anims.erase(
        std::remove_if(m_anims.begin(), m_anims.end(),
            [](const DeathAnim& a) { return !a.active; }),
        m_anims.end());
}

void DeathAnimator::draw() {
    for (auto& a : m_anims) {
        float progress = 1.0f - (a.timer / a.maxTime);
        int cx = (int)a.pos.x;
        int cy = (int)a.pos.y;

        int r1 = (int)(progress * 40);
        int alpha1 = (int)(255 * (1.0f - progress));
        setlinecolor(RGB(alpha1, alpha1 / 2, 0));
        setlinestyle(PS_SOLID, 3);
        circle(cx, cy, r1);

        int r2 = (int)(progress * 60);
        int alpha2 = (int)(180 * (1.0f - progress));
        setlinecolor(RGB(alpha2, alpha2 / 3, 0));
        setlinestyle(PS_SOLID, 2);
        circle(cx, cy, r2);

        for (int i = 0; i < 8; i++) {
            float angle = (2.0f * 3.14159265f * i) / 8 + progress * 2;
            int dist = (int)(progress * 35);
            int px = cx + (int)(std::cos(angle) * dist);
            int py = cy + (int)(std::sin(angle) * dist);
            int sz = (int)((1.0f - progress) * 4);
            if (sz > 0) {
                setfillcolor(RGB(alpha1, alpha1 / 2, 0));
                fillcircle(px, py, sz);
            }
        }
    }
}

bool DeathAnimator::isActive() const {
    for (auto& a : m_anims) {
        if (a.active) return true;
    }
    return false;
}
