#include "ParticleSystem.h"
#include <cstdio>
#include <algorithm>

void ParticleSystem::emitExplosion(Vec2 pos, COLORREF color, int count) {
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * 3.14159265f * i) / count;
        float speed = 1.5f + (rand() % 100) / 100.0f * 2.0f;
        Particle p;
        p.pos = pos;
        p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.color = color;
        p.maxLife = 0.4f + (rand() % 100) / 200.0f;
        p.life = p.maxLife;
        p.size = 3 + rand() % 3;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitHit(Vec2 pos, int damage) {
    DamageText dt;
    dt.pos = { pos.x, pos.y - 10.0f };
    dt.damage = damage;
    dt.life = 1.0f;
    dt.color = (damage >= 30) ? RGB(255, 30, 30) :      // 高伤害亮红
               (damage >= 20) ? RGB(255, 120, 50) :      // 中伤害橙红
                                RGB(255, 200, 80);        // 低伤害黄
    m_dmgTexts.push_back(dt);
}

void ParticleSystem::emitShield(Vec2 pos) {
    for (int i = 0; i < 6; i++) {
        float angle = (2.0f * 3.14159265f * i) / 6 + (rand() % 100) / 100.0f;
        Particle p;
        p.pos = { pos.x + std::cos(angle) * 20, pos.y + std::sin(angle) * 20 };
        p.vel = { std::cos(angle) * 0.5f, std::sin(angle) * 0.5f };
        p.color = RGB(100, 200, 255);
        p.maxLife = 0.3f;
        p.life = 0.3f;
        p.size = 2;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitSprintTrail(Vec2 pos, float angle) {
    Particle p;
    p.pos = { pos.x - std::cos(angle) * 15, pos.y - std::sin(angle) * 15 };
    p.vel = { 0, 0 };
    p.color = RGB(255, 200, 50);
    p.maxLife = 0.25f;
    p.life = 0.25f;
    p.size = 3;
    m_particles.push_back(p);
}

// ========== 技能激活特效 ==========

void ParticleSystem::emitShieldActivate(Vec2 pos) {
    // 扩散的能量光环
    for (int ring = 0; ring < 3; ring++) {
        for (int i = 0; i < 16; i++) {
            float angle = (2.0f * 3.14159265f * i) / 16;
            float dist = 20.0f + ring * 10.0f;
            Particle p;
            p.pos = { pos.x + std::cos(angle) * dist, pos.y + std::sin(angle) * dist };
            p.vel = { std::cos(angle) * (2.0f + ring * 1.5f),
                      std::sin(angle) * (2.0f + ring * 1.5f) };
            p.color = RGB(80, 180, 255);
            p.maxLife = 0.5f - ring * 0.1f;
            p.life = p.maxLife;
            p.size = 4 - ring;
            m_particles.push_back(p);
        }
    }
    // 中心闪光
    for (int i = 0; i < 8; i++) {
        float angle = (rand() % 360) * 3.14159265f / 180.0f;
        Particle p;
        p.pos = pos;
        p.vel = { std::cos(angle) * 3.0f, std::sin(angle) * 3.0f };
        p.color = RGB(200, 230, 255);
        p.maxLife = 0.35f;
        p.life = p.maxLife;
        p.size = 5;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitShieldDeactivate(Vec2 pos) {
    // 护盾碎片
    for (int i = 0; i < 20; i++) {
        float angle = (2.0f * 3.14159265f * i) / 20;
        float speed = 1.0f + (rand() % 100) / 50.0f;
        Particle p;
        p.pos = { pos.x + std::cos(angle) * 22, pos.y + std::sin(angle) * 22 };
        p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.color = (i % 2 == 0) ? RGB(100, 200, 255) : RGB(150, 220, 255);
        p.maxLife = 0.5f + (rand() % 100) / 200.0f;
        p.life = p.maxLife;
        p.size = 3 + rand() % 3;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitSprintActivate(Vec2 pos) {
    // 速度线爆发 - 向后方喷射
    for (int i = 0; i < 15; i++) {
        float baseAngle = (rand() % 60 - 30) * 3.14159265f / 180.0f;
        float speed = 4.0f + (rand() % 100) / 33.0f;
        Particle p;
        p.pos = pos;
        // 固定朝左/右喷射（根据出生点）
        float dir = (pos.x < 600.0f) ? 1.0f : -1.0f;
        p.vel = { -dir * std::cos(baseAngle) * speed,
                   std::sin(baseAngle) * speed };
        p.color = RGB(255, 180 + rand() % 75, 40 + rand() % 60);
        p.maxLife = 0.3f + (rand() % 100) / 200.0f;
        p.life = p.maxLife;
        p.size = 4 + rand() % 4;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitScatterMuzzle(Vec2 pos, float angle) {
    // 枪口扇形闪光
    for (int i = 0; i < 12; i++) {
        float spread = (i - 5.5f) * 3.0f * 3.14159265f / 180.0f; // ±15度扇形
        float speed = 3.0f + (rand() % 100) / 50.0f;
        Particle p;
        p.pos = pos;
        p.vel = { std::cos(angle + spread) * speed,
                  std::sin(angle + spread) * speed };
        p.color = (i < 6) ? RGB(255, 180, 40) : RGB(255, 140, 20);
        p.maxLife = 0.25f;
        p.life = p.maxLife;
        p.size = 3 + rand() % 3;
        m_particles.push_back(p);
    }
    // 中心火球
    for (int i = 0; i < 5; i++) {
        Particle p;
        p.pos = pos;
        float a = (rand() % 360) * 3.14159265f / 180.0f;
        float s = 1.0f + (rand() % 100) / 50.0f;
        p.vel = { std::cos(a) * s, std::sin(a) * s };
        p.color = RGB(255, 255, 200);
        p.maxLife = 0.2f;
        p.life = p.maxLife;
        p.size = 5;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitBulletTrail(Vec2 pos, COLORREF color, float size) {
    Particle p;
    p.pos = pos;
    p.vel = { 0, 0 };
    p.color = color;
    p.maxLife = 0.15f;
    p.life = p.maxLife;
    p.size = (int)size;
    m_particles.push_back(p);
}

void ParticleSystem::update(float dt) {
    for (auto& p : m_particles) {
        p.pos.x += p.vel.x;
        p.pos.y += p.vel.y;
        p.vel.x *= 0.95f;
        p.vel.y *= 0.95f;
        p.life -= dt;
    }
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
            [](const Particle& p) { return p.life <= 0; }),
        m_particles.end());

    for (auto& dt2 : m_dmgTexts) {
        dt2.pos.y -= 0.8f;
        dt2.life -= dt;
    }
    m_dmgTexts.erase(
        std::remove_if(m_dmgTexts.begin(), m_dmgTexts.end(),
            [](const DamageText& d) { return d.life <= 0; }),
        m_dmgTexts.end());
}

void ParticleSystem::draw() {
    for (auto& p : m_particles) {
        float alpha = p.life / p.maxLife;
        int r = (int)(GetRValue(p.color) * alpha);
        int g = (int)(GetGValue(p.color) * alpha);
        int b = (int)(GetBValue(p.color) * alpha);
        setfillcolor(RGB(r, g, b));
        fillcircle((int)p.pos.x, (int)p.pos.y, p.size);
    }

    for (auto& dt2 : m_dmgTexts) {
        float alpha = dt2.life;
        int r = (int)(GetRValue(dt2.color) * alpha);
        int g = (int)(GetGValue(dt2.color) * alpha);
        int b = (int)(GetBValue(dt2.color) * alpha);
        settextcolor(RGB(r, g, b));
        settextstyle(16, 0, "黑体");
        char text[16];
        snprintf(text, sizeof(text), "-%d", dt2.damage);
        outtextxy((int)dt2.pos.x, (int)dt2.pos.y, text);
    }
}
