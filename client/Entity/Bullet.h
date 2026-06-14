// Bullet.h
#pragma once

#include "Protocol.h"
#include "Config.h"
#include <deque>

class Bullet {
public:
    Bullet(Vec2 pos, Vec2 vel, int owner, int damage, BulletType type = BulletType::NORMAL);
    ~Bullet() = default;

    void update(float dt);
    bool isAlive() const { return m_alive; }
    Vec2 getPos()  const { return m_pos; }
    Vec2 getVel()  const { return m_vel; }
    int  getOwner() const { return m_owner; }
    int  getDamage() const { return m_damage; }
    BulletType getType() const { return m_type; }

    // 拖尾位置（用于渲染尾迹）
    const std::deque<Vec2>& getTrail() const { return m_trail; }

    // 命中后调用
    void destroy() { m_alive = false; }

private:
    Vec2       m_pos;
    Vec2       m_vel;
    int        m_owner;    // 所属玩家ID
    int        m_damage;   // 伤害值
    BulletType m_type;     // 子弹类型（普通/散射）
    bool       m_alive = true;

    // 拖尾记录
    std::deque<Vec2> m_trail;
    float   m_trailTimer = 0.0f;
    static constexpr int   MAX_TRAIL = 8;
    static constexpr float TRAIL_INTERVAL = 0.02f;
};
