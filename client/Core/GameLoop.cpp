// GameLoop.cpp
#include "GameLoop.h"
#include <iostream>

GameLoop::GameLoop() {}

GameLoop::~GameLoop() {
    stop();
}

void GameLoop::init() {
    // ===== 创建本地坦克（玩家1，出生在左边）=====
    Vec2 playerStart = { 150.0f, (float)MapConfig::HEIGHT / 2 };
    m_localTank.reset(Tank::create(0, m_localTankType, playerStart));

    // ===== 创建敌方坦克（单人模式：固定重型）=====
    Vec2 enemyStart = { (float)MapConfig::WIDTH - 150.0f, (float)MapConfig::HEIGHT / 2 };
    m_enemyTank.reset(Tank::create(1, TankType::HEAVY, enemyStart));

    // ===== 清空子弹 ======
    m_bullets.clear();

    // ===== 状态重置 ======
    m_gameOver = false;
    m_running = false;

    std::cout << "[GameLoop] 初始化完成！" << std::endl;
}

void GameLoop::start() {
    init();
    m_running = true;
    std::cout << "[GameLoop] 游戏开始！" << std::endl;
}

void GameLoop::stop() {
    m_running = false;
}

void GameLoop::update(float dt) {
    if (!m_running || m_gameOver) return;

    // ===== 读取输入 ======
    InputState input = m_input.getState();

    // ===== 更新坦克 ======
    if (m_localTank) m_localTank->applyInput(input);
    if (m_enemyTank) m_enemyTank->applyInput(input); // 单机模式：敌人也响应输入（WASD）

    // ===== 更新子弹 ======
    updateBullets(dt);

    // ===== 碰撞检测 ======
    checkCollisions();

    // ===== 技能（F键）=====
    if (input.f && m_localTank) {
        m_localTank->useSkill();
    }

    // ===== 游戏结束判定 ======
    checkGameOver();
}

void GameLoop::updateBullets(float dt) {
    for (auto& bullet : m_bullets) {
        bullet->update(dt);
    }
    // 移除死亡的子弹
    m_bullets.erase(
        std::remove_if(m_bullets.begin(), m_bullets.end(),
            [](const std::unique_ptr<Bullet>& b) { return !b->isAlive(); }),
        m_bullets.end()
    );
}

void GameLoop::checkCollisions() {
    for (auto& bullet : m_bullets) {
        if (!bullet->isAlive()) continue;

        // 子弹 vs 玩家坦克
        if (m_localTank && Collision::bulletHitsTank(*bullet, *m_localTank)) {
            if (bullet->getOwner() != m_localTank->getPlayerID()) { // 不是自己打的
                m_localTank->takeDamage(bullet->getDamage());
                bullet->destroy();
                std::cout << "[碰撞] 玩家受伤，剩余HP: " << m_localTank->getCurHP() << std::endl;
                continue;
            }
        }

        // 子弹 vs 敌人坦克
        if (m_enemyTank && Collision::bulletHitsTank(*bullet, *m_enemyTank)) {
            if (bullet->getOwner() != m_enemyTank->getPlayerID()) {
                m_enemyTank->takeDamage(bullet->getDamage());
                bullet->destroy();
                std::cout << "[碰撞] 敌人受伤，剩余HP: " << m_enemyTank->getCurHP() << std::endl;
            }
        }
    }
}

void GameLoop::checkGameOver() {
    if (!m_localTank || !m_enemyTank) return;

    if (!m_localTank->isAlive()) {
        m_gameOver = true;
        std::cout << "[GameLoop] 你输了！" << std::endl;
    } else if (!m_enemyTank->isAlive()) {
        m_gameOver = true;
        std::cout << "[GameLoop] 你赢了！" << std::endl;
    }
}

void GameLoop::fireBullet() {
    if (!m_localTank || !m_localTank->canShoot()) return;

    // 根据坦克朝向计算子弹方向
    Vec2 dir = m_localTank->getPos(); // 临时存一下

    // 子弹从坦克前方发射
    float bx = m_localTank->getPos().x + std::cos(m_localTank->getAngle()) * 20.0f;
    float by = m_localTank->getPos().y + std::sin(m_localTank->getAngle()) * 20.0f;

    float speed = m_localTank->getBulletSpeed();
    Vec2 vel = {
        std::cos(m_localTank->getAngle()) * speed,
        std::sin(m_localTank->getAngle()) * speed
    };

    m_bullets.emplace_back(
        std::make_unique<Bullet>(Vec2{ bx, by }, vel,
            m_localTank->getPlayerID(), m_localTank->getDamage())
    );

    m_localTank->resetShootTimer();
}
