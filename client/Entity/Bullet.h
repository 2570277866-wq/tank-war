// Bullet.h
#pragma once

#include "Protocol.h"
#include "Config.h"

class Bullet {
public:
    Bullet(Vec2 pos, Vec2 vel, int owner, int damage);
    ~Bullet() = default;

    void update(float dt);
    bool isAlive() const { return m_alive; }
    Vec2 getPos()  const { return m_pos; }
    Vec2 getVel()  const { return m_vel; }
    int  getOwner() const { return m_owner; }
    int  getDamage() const { return m_damage; }

    // 命中后调用
    void destroy() { m_alive = false; }

private:
    Vec2  m_pos;
    Vec2  m_vel;
    int   m_owner;    // 所属玩家ID
    int   m_damage;    // 伤害值
    bool  m_alive = true;
};
