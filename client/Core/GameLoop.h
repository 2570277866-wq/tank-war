#pragma once

#include <WinSock2.h>
#include "Protocol.h"
#include "Config.h"
#include "Entity/Tank.h"
#include "Entity/Bullet.h"
#include "Core/InputHandler.h"
#include "Core/StateInterpolator.h"
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
    void onHitReceived(const HitData& hit);  // 服务端确认命中后播放粒子

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

    // 双插值器：本地+敌人都在快照间平滑过渡
    StateInterpolator m_localInterp;
    StateInterpolator m_enemyInterp;
    bool              m_firstSnapDone = false;

    // 客户端位置预测：本地输入立即响应，服务端快照校正
    Vec2  m_predictionError = {0.0f, 0.0f};  // 累积预测误差（服务端位置 - 预测位置）

    // 自适应插值：跟踪实际快照间隔
    float m_timeSinceLastSnap = 0.0f;

    // 心跳与输入节流（服务端 20Hz tick，客户端无需 60fps 全发）
    float m_heartbeatTimer = 0.0f;
    float m_inputSendTimer = 0.0f;
};
