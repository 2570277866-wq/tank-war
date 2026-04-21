// Tank.h
#pragma once

#include "../../Common/Protocol.h"
#include "../../Common/Config.h"
#include <cmath>

class Bullet; // 前向声明

class Tank {
public:
    Tank(int playerID, TankType type, Vec2 startPos);
    virtual ~Tank() = default;

    // ============ 核心更新 ============
    void update(float dt);
    void applyInput(const InputState& input);

    // ============ 状态查询 ============
    Vec2         getPos()     const { return m_pos; }
    float        getAngle()   const { return m_angle; }
    int          getCurHP()   const { return m_curHP; }
    int          getMaxHP()   const { return m_maxHP; }
    int          getPlayerID() const { return m_playerID; }
    TankType     getType()    const { return m_type; }
    bool         isAlive()    const { return m_alive; }
    bool         isShieldActive()  const { return m_shieldActive; }
    bool         isSprintActive()  const { return m_sprintActive; }
    float        getSkillCooldown() const { return m_skillCooldown; }

    // ============ 操作 ============
    void takeDamage(int dmg);    // 被击中扣血（服务端调用）
    void useSkill();             // 释放技能
    bool canShoot() const;       // 技能CD好了才能射
    float getShootTimer() const { return m_shootTimer; }
    void resetShootTimer() { m_shootTimer = 0.3f; } // 射击间隔 0.3s

    // ============ 工厂方法 ============
    static Tank* create(int playerID, TankType type, Vec2 startPos);

    // ============ 子类钩子 ============
    virtual void onSkillStart() {}
    virtual void onSkillEnd() {}
    virtual SkillType getSkillType() const = 0;
    virtual int getDamage()     const = 0;
    virtual float getBulletSpeed() const = 0;
    virtual float getSpeed()    const = 0;
    virtual float getRotateSpeed() const = 0;
    virtual float getSkillDuration() const = 0;
    virtual float getSkillCooldownTime() const = 0;

protected:
    Vec2    m_pos;           // 位置
    Vec2    m_dir;           // 朝向单位向量（根据 m_angle 计算）
    float   m_angle = 0.0f;  // 角度（弧度），0 = 向右
    int     m_curHP = 100;
    int     m_maxHP = 100;
    int     m_playerID = 0;
    TankType m_type = TankType::LIGHT;
    bool    m_alive = true;

    // 技能相关
    float   m_skillCooldown = 0.0f;  // 当前CD计时
    bool    m_shieldActive = false;
    bool    m_sprintActive = false;
    float   m_shootTimer = 0.0f;      // 射击间隔计时

    // ====== 受保护的方法，子类可覆盖 ======
    void _activateShield();
    void _activateSprint();
    void _deactivateShield();
    void _deactivateSprint();
    void _updateDirection();
};

// ============ Heavy Tank ============
class HeavyTank : public Tank {
public:
    HeavyTank(int playerID, TankType type, Vec2 startPos);

    SkillType getSkillType()    const override { return SkillType::SHIELD; }
    int       getDamage()       const override { return TankConfig::HEAVY.damage; }
    float     getBulletSpeed()  const override { return TankConfig::HEAVY.bulletSpeed; }
    float     getSpeed()        const override { return TankConfig::HEAVY.speed; }
    float     getRotateSpeed()  const override { return TankConfig::HEAVY.rotateSpeed; }
    float     getSkillDuration() const override { return TankConfig::HEAVY.skillDuration; }
    float     getSkillCooldownTime() const override { return TankConfig::HEAVY.skillCooldown; }

    void onSkillStart() override;
    void onSkillEnd() override;
};

// ============ Light Tank ============
class LightTank : public Tank {
public:
    LightTank(int playerID, TankType type, Vec2 startPos);

    SkillType getSkillType()    const override { return SkillType::SPRINT; }
    int       getDamage()       const override { return TankConfig::LIGHT.damage; }
    float     getBulletSpeed()  const override { return TankConfig::LIGHT.bulletSpeed; }
    float     getSpeed()        const override { return TankConfig::LIGHT.speed; }
    float     getRotateSpeed()  const override { return TankConfig::LIGHT.rotateSpeed; }
    float     getSkillDuration() const override { return TankConfig::LIGHT.skillDuration; }
    float     getSkillCooldownTime() const override { return TankConfig::LIGHT.skillCooldown; }

    void onSkillStart() override;
    void onSkillEnd() override;
};

// ============ Scout Tank ============
class ScoutTank : public Tank {
public:
    ScoutTank(int playerID, TankType type, Vec2 startPos);

    SkillType getSkillType()    const override { return SkillType::SCATTER; }
    int       getDamage()       const override { return TankConfig::SCOUT.damage; }
    float     getBulletSpeed()  const override { return TankConfig::SCOUT.bulletSpeed; }
    float     getSpeed()        const override { return TankConfig::SCOUT.speed; }
    float     getRotateSpeed()  const override { return TankConfig::SCOUT.rotateSpeed; }
    float     getSkillDuration() const override { return TankConfig::SCOUT.skillDuration; }
    float     getSkillCooldownTime() const override { return TankConfig::SCOUT.skillCooldown; }

    void onSkillStart() override;
    void onSkillEnd() override;
};
