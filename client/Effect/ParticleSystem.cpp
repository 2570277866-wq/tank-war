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
    dt.color = RGB(255, 60, 60);
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
        int r = (GetRValue(p.color) * alpha);
        int g = (GetGValue(p.color) * alpha);
        int b = (GetBValue(p.color) * alpha);
        setfillcolor(RGB(r, g, b));
        fillcircle((int)p.pos.x, (int)p.pos.y, p.size);
    }

    for (auto& dt2 : m_dmgTexts) {
        float alpha = dt2.life;
        int r = (GetRValue(dt2.color) * alpha);
        int g = (GetGValue(dt2.color) * alpha);
        int b = (GetBValue(dt2.color) * alpha);
        settextcolor(RGB(r, g, b));
        settextstyle(16, 0, "黑体");
        char text[16];
        snprintf(text, sizeof(text), "-%d", dt2.damage);
        outtextxy((int)dt2.pos.x, (int)dt2.pos.y, text);
    }
}
