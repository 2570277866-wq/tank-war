#pragma once

#include <WinSock2.h>
#include "Protocol.h"
#include "Config.h"
#include "Entity/Tank.h"
#include "Entity/Bullet.h"
#include "Core/InputHandler.h"
#include "Effect/Collision.h"
#include "Effect/ParticleSystem.h"
#include "Effect/GameMap.h"
#include "Effect/DeathAnimator.h"
#include "UI/Minimap.h"
#include "UI/GameOverUI.h"
#include "Net/NetClient.h"
#include "Net/MsgCodec.h"
#include <graphics.h>
#include <vector>
#include <memory>

class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    void init();
    void initFromMatch(const MatchResultData& match, int localPlayerID);
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
    int      m_localPlayerID = -1;

    std::unique_ptr<Tank>  m_localTank;
    std::unique_ptr<Tank>  m_enemyTank;
    std::vector<std::unique_ptr<Bullet>> m_bullets;

    InputHandler    m_input;
    NetClient*      m_netClient = nullptr;
    ParticleSystem  m_particles;
    GameMap         m_map;
    DeathAnimator   m_deathAnim;

    int     m_totalDamage = 0;
    int     m_kills = 0;
    float   m_gameTime = 0.0f;

    bool     m_prevSpace = false;
    bool     m_prevF = false;
    uint32_t m_lastSnapSeq = 0;
};
