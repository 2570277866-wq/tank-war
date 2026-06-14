#pragma once

#include "Protocol.h"
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

    // 技能激活特效
    void emitShieldActivate(Vec2 pos);     // 护盾激活：扩散光环
    void emitShieldDeactivate(Vec2 pos);   // 护盾消失：碎片粒子
    void emitSprintActivate(Vec2 pos);     // 冲刺激活：速度线爆发
    void emitScatterMuzzle(Vec2 pos, float angle); // 散射枪口闪光

    // 子弹特效
    void emitBulletTrail(Vec2 pos, COLORREF color, float size); // 子弹拖尾粒子

    void update(float dt);
    void draw();

    void clear() { m_particles.clear(); m_dmgTexts.clear(); }

private:
    std::vector<Particle>   m_particles;
    std::vector<DamageText> m_dmgTexts;
};
