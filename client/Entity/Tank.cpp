// Tank.cpp
#include "Tank.h"
#include <algorithm>

// ============ 常量 ============
constexpr float TANK_RADIUS = 15.0f;
constexpr float PI = 3.14159265358979f;

// ============ Tank 基类 ============
Tank::Tank(int playerID, TankType type, Vec2 startPos)
    : m_pos(startPos)
    , m_playerID(playerID)
    , m_type(type)
{
    m_dir = { 1.0f, 0.0f }; // 默认朝右
    m_angle = 0.0f;
    m_skillCooldown = 0.0f;
    m_shieldActive = false;
    m_sprintActive = false;
    m_shootTimer = 0.0f;
    m_alive = true;

    // 从配置读取属性
    switch (type) {
        case TankType::HEAVY:
            m_maxHP = TankConfig::HEAVY.maxHP;
            m_curHP = m_maxHP;
            break;
        case TankType::LIGHT:
            m_maxHP = TankConfig::LIGHT.maxHP;
            m_curHP = m_maxHP;
            break;
        case TankType::SCOUT:
            m_maxHP = TankConfig::SCOUT.maxHP;
            m_curHP = m_maxHP;
            break;
    }
}

Tank* Tank::create(int playerID, TankType type, Vec2 startPos) {
    switch (type) {
        case TankType::HEAVY: return new HeavyTank(playerID, type, startPos);
        case TankType::LIGHT: return new LightTank(playerID, type, startPos);
        case TankType::SCOUT: return new ScoutTank(playerID, type, startPos);
        default: return new LightTank(playerID, TankType::LIGHT, startPos);
    }
}

void Tank::_updateDirection() {
    m_dir.x = std::cos(m_angle);
    m_dir.y = std::sin(m_angle);
}

void Tank::applyInput(const InputState& input) {
    if (!m_alive) return;

    // ===== 转向（WASD左右）=====
    float rotSpd = getRotateSpeed();
    if (input.a) m_angle -= rotSpd * 0.016f; // 假设 60fps, dt≈16ms
    if (input.d) m_angle += rotSpd * 0.016f;
    _updateDirection();

    // ===== 移动（WASD前后）=====
    float spd = getSpeed();
    if (m_sprintActive) spd *= 2.0f; // 冲刺速度翻倍

    Vec2 moveDir = { 0, 0 };
    if (input.w) { moveDir.x += m_dir.x; moveDir.y += m_dir.y; }
    if (input.s) { moveDir.x -= m_dir.x; moveDir.y -= m_dir.y; }

    // 归一化（防止斜向速度过快）
    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.001f) {
        moveDir.x /= len;
        moveDir.y /= len;
    }

    m_pos.x += moveDir.x * spd;
    m_pos.y += moveDir.y * spd;

    // ===== 边界限制 ======
    m_pos.x = std::max(TANK_RADIUS, std::min((float)MapConfig::WIDTH  - TANK_RADIUS, m_pos.x));
    m_pos.y = std::max(TANK_RADIUS, std::min((float)MapConfig::HEIGHT - TANK_RADIUS, m_pos.y));
}

void Tank::update(float dt) {
    if (!m_alive) return;

    if (m_skillCooldown > 0) m_skillCooldown -= dt;
    if (m_skillCooldown < 0) m_skillCooldown = 0;

    if (m_skillTimer > 0) {
        m_skillTimer -= dt;
        if (m_skillTimer <= 0) {
            m_skillTimer = 0;
            onSkillEnd();
        }
    }

    if (m_shootTimer > 0) m_shootTimer -= dt;
}

bool Tank::canShoot() const {
    return m_alive && m_shootTimer <= 0;
}

void Tank::takeDamage(int dmg) {
    if (m_shieldActive) return; // 有护盾不掉血
    if (!m_alive) return;
    m_curHP -= dmg;
    if (m_curHP <= 0) {
        m_curHP = 0;
        m_alive = false;
    }
}

void Tank::useSkill() {
    if (!m_alive) return;
    if (m_skillCooldown > 0) return;

    m_skillCooldown = getSkillCooldownTime();
    m_skillTimer = getSkillDuration();
    onSkillStart();
}

// ============ Heavy Tank ============
HeavyTank::HeavyTank(int playerID, TankType type, Vec2 startPos)
    : Tank(playerID, type, startPos) {}

void HeavyTank::onSkillStart() { _activateShield(); }
void HeavyTank::onSkillEnd()   { _deactivateShield(); }

// ============ Light Tank ============
LightTank::LightTank(int playerID, TankType type, Vec2 startPos)
    : Tank(playerID, type, startPos) {}

void LightTank::onSkillStart() { _activateSprint(); }
void LightTank::onSkillEnd()   { _deactivateSprint(); }

// ============ Scout Tank ============
ScoutTank::ScoutTank(int playerID, TankType type, Vec2 startPos)
    : Tank(playerID, type, startPos) {}

void ScoutTank::onSkillStart() { /* 弹幕散射：服务端生成3颗子弹 */ }
void ScoutTank::onSkillEnd()   {}

// ====== 私有辅助方法实现 ======
void Tank::_activateShield()  { m_shieldActive = true; }
void Tank::_activateSprint() { m_sprintActive = true; }
void Tank::_deactivateShield() { m_shieldActive = false; }
void Tank::_deactivateSprint() { m_sprintActive = false; }
