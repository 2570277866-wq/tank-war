#include "GameLoop.h"
#include <iostream>
#include <cmath>

constexpr float TANK_RADIUS = 15.0f;
constexpr float PI = 3.14159265358979f;

GameLoop::GameLoop() {}

GameLoop::~GameLoop() {
    stop();
}

void GameLoop::init() {
    Vec2 playerStart = { 150.0f, (float)MapConfig::HEIGHT / 2 };
    m_localTank.reset(Tank::create(0, m_localTankType, playerStart));

    Vec2 enemyStart = { (float)MapConfig::WIDTH - 150.0f, (float)MapConfig::HEIGHT / 2 };
    m_enemyTank.reset(Tank::create(1, TankType::HEAVY, enemyStart));

    m_bullets.clear();
    m_gameOver = false;
    m_running = false;
    m_prevSpace = false;
    m_prevF = false;
}

void GameLoop::start() {
    init();
    m_running = true;
}

void GameLoop::stop() {
    m_running = false;
}

void GameLoop::update(float dt) {
    if (!m_running || m_gameOver) return;

    m_input.update();
    InputState input = m_input.getState();

    if (m_localTank) m_localTank->applyInput(input);

    if (m_localTank) m_localTank->update(dt);
    if (m_enemyTank) m_enemyTank->update(dt);

    bool spacePressed = input.space && !m_prevSpace;
    bool fPressed = input.f && !m_prevF;
    m_prevSpace = input.space;
    m_prevF = input.f;

    if (spacePressed) fireBullet();
    if (fPressed && m_localTank) m_localTank->useSkill();

    if (m_netClient && m_netClient->isConnected()) {
        char buf[256];
        uint16_t len;
        MsgCodec::encodeInput(input, buf, len);
        m_netClient->sendMsg(MsgID::C2S_INPUT, buf, len);
    }

    updateBullets(dt);
    checkCollisions();
    checkGameOver();

    m_particles.update(dt);
    m_deathAnim.update(dt);

    if (m_running && !m_gameOver) {
        m_gameTime += dt;
    }
}

void GameLoop::updateBullets(float dt) {
    for (auto& bullet : m_bullets) {
        bullet->update(dt);
    }
    m_bullets.erase(
        std::remove_if(m_bullets.begin(), m_bullets.end(),
            [](const std::unique_ptr<Bullet>& b) { return !b->isAlive(); }),
        m_bullets.end()
    );
}

void GameLoop::checkCollisions() {
    for (auto& bullet : m_bullets) {
        if (!bullet->isAlive()) continue;

        if (m_map.checkBulletCollision(bullet->getPos(), 4.0f, bullet->getDamage())) {
            m_particles.emitExplosion(bullet->getPos(), RGB(200, 150, 50), 6);
            bullet->destroy();
            continue;
        }

        if (m_localTank && Collision::bulletHitsTank(*bullet, *m_localTank)) {
            if (bullet->getOwner() != m_localTank->getPlayerID()) {
                m_localTank->takeDamage(bullet->getDamage());
                m_particles.emitExplosion(bullet->getPos(), RGB(255, 150, 50));
                m_particles.emitHit(bullet->getPos(), bullet->getDamage());
                m_totalDamage += bullet->getDamage();
                bullet->destroy();
                continue;
            }
        }

        if (m_enemyTank && Collision::bulletHitsTank(*bullet, *m_enemyTank)) {
            if (bullet->getOwner() != m_enemyTank->getPlayerID()) {
                m_enemyTank->takeDamage(bullet->getDamage());
                m_particles.emitExplosion(bullet->getPos(), RGB(255, 150, 50));
                m_particles.emitHit(bullet->getPos(), bullet->getDamage());
                m_totalDamage += bullet->getDamage();
                if (!m_enemyTank->isAlive()) m_kills++;
                bullet->destroy();
            }
        }
    }
}

void GameLoop::checkGameOver() {
    if (!m_localTank || !m_enemyTank) return;

    if (!m_localTank->isAlive()) {
        m_gameOver = true;
    } else if (!m_enemyTank->isAlive()) {
        m_gameOver = true;
    }
}

void GameLoop::applySnapshot(const Snapshot& snap) {
    if (snap.frameSeq <= m_lastSnapSeq) return;
    m_lastSnapSeq = snap.frameSeq;

    for (int i = 0; i < 2; i++) {
        const TankState& ts = snap.tanks[i];
        Tank* tank = (ts.playerID == 0) ? m_localTank.get() : m_enemyTank.get();
        if (!tank) continue;

        tank->setPosFromSnapshot(ts);
    }

    m_bullets.clear();
    for (int i = 0; i < snap.bulletCount && i < MAX_BULLETS; i++) {
        const BulletState& bs = snap.bullets[i];
        m_bullets.emplace_back(
            std::make_unique<Bullet>(bs.pos, bs.vel, bs.owner, bs.damage)
        );
    }
}

void GameLoop::fireBullet() {
    if (!m_localTank || !m_localTank->canShoot()) return;

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

    if (m_netClient && m_netClient->isConnected() && m_localTank) {
        char buf[256];
        uint16_t len;
        MsgCodec::encodeShoot(m_localTank->getPlayerID(), m_localTank->getPos(), m_localTank->getAngle(), buf, len);
        m_netClient->sendMsg(MsgID::C2S_SHOOT, buf, len);
    }
}

// ============ 渲染 ============

void GameLoop::drawMap() {
    setfillcolor(RGB(40, 55, 40));
    fillrectangle(0, 0, MapConfig::WIDTH, MapConfig::HEIGHT);

    setlinecolor(RGB(100, 100, 100));
    setlinestyle(PS_SOLID, 2);
    rectangle(0, 0, MapConfig::WIDTH - 1, MapConfig::HEIGHT - 1);

    m_map.draw();
}

void GameLoop::drawTank(Tank* tank, COLORREF bodyColor, COLORREF turretColor) {
    if (!tank || !tank->isAlive()) return;

    int cx = (int)tank->getPos().x;
    int cy = (int)tank->getPos().y;
    float angle = tank->getAngle();

    int bodyW = 30, bodyH = 22;
    float cosA = std::cos(angle), sinA = std::sin(angle);

    int corners[4][2];
    int hw = bodyW / 2, hh = bodyH / 2;
    int localCorners[4][2] = {
        { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh }
    };
    for (int i = 0; i < 4; i++) {
        corners[i][0] = cx + (int)(localCorners[i][0] * cosA - localCorners[i][1] * sinA);
        corners[i][1] = cy + (int)(localCorners[i][0] * sinA + localCorners[i][1] * cosA);
    }

    POINT pts[4];
    for (int i = 0; i < 4; i++) { pts[i].x = corners[i][0]; pts[i].y = corners[i][1]; }
    setfillcolor(bodyColor);
    setlinecolor(RGB(30, 30, 30));
    fillpolygon(pts, 4);

    int turretR = 8;
    setfillcolor(turretColor);
    fillcircle(cx, cy, turretR);

    int barrelLen = 18;
    int bx = cx + (int)(cosA * barrelLen);
    int by = cy + (int)(sinA * barrelLen);
    setlinecolor(turretColor);
    setlinestyle(PS_SOLID, 4);
    line(cx, cy, bx, by);

    if (tank->isShieldActive()) {
        setlinecolor(RGB(100, 200, 255));
        setlinestyle(PS_SOLID, 2);
        circle(cx, cy, 22);
    }

    if (tank->isSprintActive()) {
        setlinecolor(RGB(255, 200, 50));
        setlinestyle(PS_SOLID, 1);
        for (int i = 0; i < 3; i++) {
            int trailX = cx - (int)(cosA * (10 + i * 6));
            int trailY = cy - (int)(sinA * (10 + i * 6));
            circle(trailX, trailY, 3 - i);
        }
    }
}

void GameLoop::drawBullets() {
    for (auto& bullet : m_bullets) {
        if (!bullet->isAlive()) continue;
        int bx = (int)bullet->getPos().x;
        int by = (int)bullet->getPos().y;
        COLORREF color = (bullet->getOwner() == 0) ? RGB(255, 255, 100) : RGB(255, 100, 100);
        setfillcolor(color);
        fillcircle(bx, by, 4);
    }
}

void GameLoop::drawHUD() {
    if (m_localTank) {
        int x = 10, y = 10;
        settextstyle(18, 0, L"黑体");
        settextcolor(WHITE);
        outtextxy(x, y, L"玩家");

        int barW = 120, barH = 12;
        float ratio = (float)m_localTank->getCurHP() / m_localTank->getMaxHP();
        setfillcolor(RGB(60, 60, 60));
        fillrectangle(x, y + 22, x + barW, y + 22 + barH);
        setfillcolor(ratio > 0.3f ? RGB(50, 200, 50) : RGB(200, 50, 50));
        fillrectangle(x, y + 22, x + (int)(barW * ratio), y + 22 + barH);

        wchar_t hpText[32];
        swprintf_s(hpText, L"HP:%d/%d", m_localTank->getCurHP(), m_localTank->getMaxHP());
        settextstyle(14, 0, L"宋体");
        settextcolor(WHITE);
        outtextxy(x + barW + 5, y + 20, hpText);

        if (m_localTank->getSkillCooldown() > 0) {
            wchar_t cdText[32];
            swprintf_s(cdText, L"技能CD:%.1fs", m_localTank->getSkillCooldown());
            settextcolor(RGB(200, 200, 200));
            outtextxy(x, y + 40, cdText);
        } else {
            settextcolor(RGB(100, 255, 100));
            outtextxy(x, y + 40, L"技能就绪[F]");
        }
    }

    if (m_enemyTank) {
        int x = MapConfig::WIDTH - 170, y = 10;
        settextstyle(18, 0, L"黑体");
        settextcolor(WHITE);
        outtextxy(x, y, L"敌人");

        int barW = 120, barH = 12;
        float ratio = (float)m_enemyTank->getCurHP() / m_enemyTank->getMaxHP();
        setfillcolor(RGB(60, 60, 60));
        fillrectangle(x, y + 22, x + barW, y + 22 + barH);
        setfillcolor(ratio > 0.3f ? RGB(50, 200, 50) : RGB(200, 50, 50));
        fillrectangle(x, y + 22, x + (int)(barW * ratio), y + 22 + barH);

        wchar_t hpText[32];
        swprintf_s(hpText, L"HP:%d/%d", m_enemyTank->getCurHP(), m_enemyTank->getMaxHP());
        settextstyle(14, 0, L"宋体");
        settextcolor(WHITE);
        outtextxy(x + barW + 5, y + 20, hpText);
    }

    settextstyle(14, 0, L"宋体");
    settextcolor(RGB(150, 150, 150));
    outtextxy(MapConfig::WIDTH / 2 - 80, MapConfig::HEIGHT - 25, L"WASD移动 空格射击 F技能");
}

void GameLoop::draw() {
    drawMap();

    drawTank(m_localTank.get(), RGB(50, 120, 200), RGB(80, 160, 255));
    drawTank(m_enemyTank.get(), RGB(200, 60, 60), RGB(255, 80, 80));

    drawBullets();
    m_particles.draw();
    m_map.drawGrassOverlay();
    m_deathAnim.draw();
    drawHUD();

    if (m_localTank && m_enemyTank) {
        Minimap::draw(m_localTank->getPos(), m_enemyTank->getPos(),
                      m_localTank->isAlive(), m_enemyTank->isAlive());
    }

    if (m_gameOver) {
        setfillcolor(RGB(0, 0, 0));
        setfillstyle(BS_NULL);
        settextstyle(48, 0, L"黑体");

        bool playerWin = m_enemyTank && !m_enemyTank->isAlive();
        settextcolor(playerWin ? RGB(50, 255, 50) : RGB(255, 50, 50));
        const wchar_t* result = playerWin ? L"胜  利!" : L"败  北!";
        int tw = textwidth(result);
        outtextxy((MapConfig::WIDTH - tw) / 2, MapConfig::HEIGHT / 2 - 30, result);

        settextstyle(20, 0, L"宋体");
        settextcolor(RGB(200, 200, 200));
        outtextxy(MapConfig::WIDTH / 2 - 80, MapConfig::HEIGHT / 2 + 30, L"按 ESC 返回主菜单");
    }

    FlushBatchDraw();
}
