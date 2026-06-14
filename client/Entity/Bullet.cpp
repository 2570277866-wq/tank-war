#include "Bullet.h"
#include "Config.h"

Bullet::Bullet(Vec2 pos, Vec2 vel, int owner, int damage, BulletType type)
    : m_pos(pos), m_vel(vel), m_owner(owner), m_damage(damage), m_type(type) {}

void Bullet::update(float dt) {
    if (!m_alive) return;
    m_pos.x += m_vel.x * dt;
    m_pos.y += m_vel.y * dt;

    // 拖尾记录（散射子弹拖尾更密集）
    m_trailTimer += dt;
    float interval = (m_type == BulletType::SCATTER) ? TRAIL_INTERVAL * 0.5f : TRAIL_INTERVAL;
    if (m_trailTimer >= interval) {
        m_trailTimer -= interval;
        m_trail.push_back(m_pos);
        if ((int)m_trail.size() > MAX_TRAIL) m_trail.pop_front();
    }

    if (m_pos.x < 0 || m_pos.x > MapConfig::WIDTH ||
        m_pos.y < 0 || m_pos.y > MapConfig::HEIGHT) {
        m_alive = false;
    }
}
