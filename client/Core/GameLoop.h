// GameLoop.h
#pragma once

#include "../../Common/Protocol.h"
#include "Tank.h"
#include "Bullet.h"
#include "InputHandler.h"
#include "Collision.h"
#include <vector>
#include <memory>

class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    // ===== 生命周期 =====
    void init();       // 初始化（选坦克后调用）
    void start();      // 开始游戏
    void stop();       // 停止游戏

    // ===== 主循环（每帧调用）=====
    void update(float dt);

    // ===== 访问器 =====
    bool isRunning()    const { return m_running; }
    bool isGameOver()   const { return m_gameOver; }
    Tank* getLocalTank()    { return m_localTank.get(); }
    Tank* getEnemyTank()    { return m_enemyTank.get(); }
    const std::vector<std::unique_ptr<Bullet>>& getBullets() const { return m_bullets; }

    // ===== 玩家控制 ======
    void setLocalTankType(TankType type) { m_localTankType = type; }
    void fireBullet();  // 玩家按下空格时调用

private:
    void updateBullets(float dt);
    void checkCollisions();
    void checkGameOver();

    bool     m_running = false;
    bool     m_gameOver = false;
    TankType m_localTankType = TankType::LIGHT;

    std::unique_ptr<Tank>  m_localTank;   // 本地玩家坦克
    std::unique_ptr<Tank>  m_enemyTank;   // 敌人坦克（单人模式用假AI）
    std::vector<std::unique_ptr<Bullet>> m_bullets;

    InputHandler m_input;
};
