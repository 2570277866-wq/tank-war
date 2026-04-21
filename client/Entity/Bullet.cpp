// Bullet.cpp
#include "Bullet.h"
#include <cmath>

Bullet::Bullet(Vec2 pos, Vec2 vel, int owner, int damage)
    : m_pos(pos), m_vel(vel), m_owner(owner), m_damage(damage) {}

void Bullet::update(float dt) {
    if (!m_alive) return;
    m_pos.x += m_vel.x * dt;
    m_pos.y += m_vel.y * dt;

    // 出界判定
    if (m_pos.x < 0 || m_pos.x > MapConfig::WIDTH ||
        m_pos.y < 0 || m_pos.y > MapConfig::HEIGHT) {
        m_alive = false;
    }
}
