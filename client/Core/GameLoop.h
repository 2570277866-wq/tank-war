#pragma once

#include "../../Common/Protocol.h"
#include "../../Common/Config.h"
#include "Tank.h"
#include "Bullet.h"
#include "InputHandler.h"
#include "Collision.h"
#include "../Effect/ParticleSystem.h"
#include "../Net/NetClient.h"
#include "../Net/MsgCodec.h"
#include <graphics.h>
#include <vector>
#include <memory>

class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    void init();
    void start();
    void stop();

    void update(float dt);
    void draw();

    bool isRunning()    const { return m_running; }
    bool isGameOver()   const { return m_gameOver; }
    Tank* getLocalTank()    { return m_localTank.get(); }
    Tank* getEnemyTank()    { return m_enemyTank.get(); }
    const std::vector<std::unique_ptr<Bullet>>& getBullets() const { return m_bullets; }

    void setLocalTankType(TankType type) { m_localTankType = type; }
    void setNetClient(NetClient* nc) { m_netClient = nc; }
    void fireBullet();
    void applySnapshot(const Snapshot& snap);

private:
    void updateBullets(float dt);
    void checkCollisions();
    void checkGameOver();
    void drawTank(Tank* tank, COLORREF bodyColor, COLORREF turretColor);
    void drawBullets();
    void drawHUD();
    void drawMap();

    bool     m_running = false;
    bool     m_gameOver = false;
    TankType m_localTankType = TankType::LIGHT;

    std::unique_ptr<Tank>  m_localTank;
    std::unique_ptr<Tank>  m_enemyTank;
    std::vector<std::unique_ptr<Bullet>> m_bullets;

    InputHandler    m_input;
    NetClient*      m_netClient = nullptr;
    ParticleSystem  m_particles;

    bool     m_prevSpace = false;
    bool     m_prevF = false;
    uint32_t m_lastSnapSeq = 0;
};
