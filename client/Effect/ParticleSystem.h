#pragma once

#include "../../Common/Protocol.h"
#include <graphics.h>
#include <vector>
#include <cmath>

struct Particle {
    Vec2    pos;
    Vec2    vel;
    COLORREF color;
    float   life;
    float   maxLife;
    int     size;
};

struct DamageText {
    Vec2    pos;
    int     damage;
    float   life;
    COLORREF color;
};

class ParticleSystem {
public:
    void emitExplosion(Vec2 pos, COLORREF color, int count = 12);
    void emitHit(Vec2 pos, int damage);
    void emitShield(Vec2 pos);
    void emitSprintTrail(Vec2 pos, float angle);

    void update(float dt);
    void draw();

    void clear() { m_particles.clear(); m_dmgTexts.clear(); }

private:
    std::vector<Particle>   m_particles;
    std::vector<DamageText> m_dmgTexts;
};
