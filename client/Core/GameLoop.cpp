#include "GameLoop.h"
#include <iostream>
#include <cmath>
#include <algorithm>

constexpr float PI = 3.14159265358979f;

GameLoop::GameLoop() {}
GameLoop::~GameLoop() { stop(); }

void GameLoop::init() {
    m_bullets.clear();
    m_gameOver = false;
    m_running = false;
    m_prevSpace = m_prevF = false;
    m_firstSnapDone = false;
    m_lastSnapSeq = 0;
    m_predictionError = {0.0f, 0.0f};
    m_timeSinceLastSnap = 0.0f;
}

void GameLoop::initFromMatch(const MatchResultData& match, int localPlayerID) {
    m_localPlayerID = localPlayerID;

    int localSlot = (match.playerIDs[0] == localPlayerID) ? 0 : 1;
    int enemySlot = 1 - localSlot;
    int enemyID   = match.playerIDs[enemySlot];
    TankType enemyType = match.tankTypes[enemySlot];

    // 使用服务端确认的坦克类型（match 数据中 index 0 = 自己）
    TankType localTankType = match.tankTypes[localSlot];
    m_localTank.reset(Tank::create(localPlayerID, localTankType, match.startPositions[localSlot]));
    m_enemyTank.reset(Tank::create(enemyID, enemyType, match.startPositions[enemySlot]));

    // 根据出生点在左侧还是右侧决定初始朝向（左侧朝右=0，右侧朝左=PI）
    auto calcInitAngle = [](Vec2 pos) -> float {
        return (pos.x < MapConfig::WIDTH / 2.0f) ? 0.0f : PI;
    };
    float localAngle = calcInitAngle(match.startPositions[localSlot]);
    float enemyAngle = calcInitAngle(match.startPositions[enemySlot]);

    // 插值器初始化
    InterpState s0 = { match.startPositions[localSlot], localAngle };
    InterpState s1 = { match.startPositions[enemySlot], enemyAngle };
    m_localInterp.snap(s0);
    m_enemyInterp.snap(s1);

    // 坦克初始角度同步设置，避免首帧快照到达前朝向错误
    m_localTank->setAngle(localAngle);
    m_enemyTank->setAngle(enemyAngle);

    m_bullets.clear();
    m_gameOver = false;
    m_running = false;
    m_prevSpace = m_prevF = false;
    m_firstSnapDone = false;
    m_lastSnapSeq = 0;
}

void GameLoop::start() {
    init();
    m_running = true;
}

void GameLoop::stop() { m_running = false; }

void GameLoop::update(float dt) {
    if (!m_running || m_gameOver) return;

    m_input.update();
    InputState input = m_input.getState();

    // 双插值器前进
    m_timeSinceLastSnap += dt;
    m_localInterp.update(dt);
    m_enemyInterp.update(dt);

    // 从插值器读取平滑后的位置（本地和敌人都是服务器权威）
    if (m_firstSnapDone) {
        if (m_localTank) {
            InterpState s = m_localInterp.getCurrent();
            m_localTank->setPos(s.pos);
            m_localTank->setAngle(s.angle);
        }
        if (m_enemyTank) {
            InterpState s = m_enemyInterp.getCurrent();
            m_enemyTank->setPos(s.pos);
            m_enemyTank->setAngle(s.angle);
        }
    }

    // 本地角度预测（不影响位置，只是让转向手感更即时）
    if (m_localTank) {
        float r = m_localTank->getType() == TankType::HEAVY ? TankConfig::HEAVY.rotateSpeed :
                 m_localTank->getType() == TankType::LIGHT ? TankConfig::LIGHT.rotateSpeed :
                 TankConfig::SCOUT.rotateSpeed;
        if (input.a) m_localTank->setAngle(m_localTank->getAngle() - r * dt);
        if (input.d) m_localTank->setAngle(m_localTank->getAngle() + r * dt);
    }

    // 本地位置预测：基于输入立即移动，消除服务端往返延迟感
    // 配合 applySnapshot 中的误差记录，平滑校正累积偏移
    if (m_localTank && m_firstSnapDone && m_localTank->isAlive()) {
        float spd = m_localTank->getSpeed();
        if (m_localTank->isSprintActive()) spd *= 2.0f;

        float a = m_localTank->getAngle();
        Vec2 moveDir = {0.0f, 0.0f};
        if (input.w) { moveDir.x += std::cos(a); moveDir.y += std::sin(a); }
        if (input.s) { moveDir.x -= std::cos(a); moveDir.y -= std::sin(a); }

        float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (len > 0.001f) { moveDir.x /= len; moveDir.y /= len; }

        // 预测误差指数衰减（~125ms 校正周期）
        float errorDecay = 8.0f * dt;
        if (errorDecay > 1.0f) errorDecay = 1.0f;
        Vec2 correction = {
            m_predictionError.x * errorDecay,
            m_predictionError.y * errorDecay,
        };
        m_predictionError.x -= correction.x;
        m_predictionError.y -= correction.y;

        Vec2 predicted = {
            m_localTank->getPos().x + moveDir.x * spd * dt + correction.x,
            m_localTank->getPos().y + moveDir.y * spd * dt + correction.y,
        };

        // 边界钳制（与服务器 MoveTank 中 ClampToBounds 一致）
        constexpr float TR = CollisionConfig::TANK_RADIUS;
        predicted.x = std::max(TR, std::min((float)MapConfig::WIDTH - TR, predicted.x));
        predicted.y = std::max(TR, std::min((float)MapConfig::HEIGHT - TR, predicted.y));

        m_localTank->setPos(predicted);
    }

    // 计时器更新
    if (m_localTank) m_localTank->update(dt);
    if (m_enemyTank) m_enemyTank->update(dt);

    // 心跳 + 输入节流计时器
    m_heartbeatTimer += dt;
    m_inputSendTimer += dt;

    // 射击/技能
    bool spacePressed = input.space && !m_prevSpace;
    bool fPressed     = input.f && !m_prevF;
    m_prevSpace = input.space;
    m_prevF     = input.f;

    if (spacePressed) fireBullet();
    if (fPressed && m_localTank) {
        m_localTank->useSkill();
        // 通知服务端释放技能
        if (m_netClient && m_netClient->isConnected()) {
            char buf[256]; uint16_t len;
            MsgCodec::encodeSkill(m_localTank->getPlayerID(),
                                  m_localTank->getSkillType(), buf, len);
            m_netClient->sendMsg(MsgID::C2S_USE_SKILL, buf, len);
        }
    }

    // 心跳（每 ~1 秒一次，服务端 3 秒超时留足余量）
    if (m_heartbeatTimer >= 1.0f) {
        m_heartbeatTimer = 0.0f;
        if (m_netClient && m_netClient->isConnected()) {
            m_netClient->sendMsg(MsgID::C2S_HEARTBEAT);
        }
    }

    // 输入节流发送（~30Hz，避免 60fps→20Hz tick 导致服务端 CHEAT 误报堆积）
    if (m_inputSendTimer >= 0.033f) {
        m_inputSendTimer = 0.0f;
        if (m_netClient && m_netClient->isConnected()) {
            char buf[256]; uint16_t len;
            MsgCodec::encodeInput(input, buf, len);
            m_netClient->sendMsg(MsgID::C2S_INPUT, buf, len);
        }
    }

    updateBullets(dt);
    checkCollisions();
    checkGameOver();
    m_particles.update(dt);
    m_deathAnim.update(dt);

    if (m_running && !m_gameOver) m_gameTime += dt;
}

void GameLoop::updateBullets(float dt) {
    for (auto& b : m_bullets) b->update(dt);
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(),
        [](auto& b) { return !b->isAlive(); }), m_bullets.end());
}

void GameLoop::checkCollisions() {
    // 注意：子弹-坦克碰撞由服务端权威判定，客户端不做此检测
    // 客户端只处理子弹-障碍物碰撞（纯视觉效果）
    for (auto& bullet : m_bullets) {
        if (!bullet->isAlive()) continue;
        if (m_map.checkBulletCollision(bullet->getPos(), CollisionConfig::BULLET_RADIUS, bullet->getDamage())) {
            m_particles.emitExplosion(bullet->getPos(), RGB(200,150,50), 6);
            bullet->destroy();
        }
    }
}

void GameLoop::checkGameOver() {
    if (!m_localTank || !m_enemyTank) return;
    if (!m_localTank->isAlive() || !m_enemyTank->isAlive()) m_gameOver = true;
}

void GameLoop::applySnapshot(const Snapshot& snap) {
    if (snap.frameSeq <= m_lastSnapSeq) return;
    m_lastSnapSeq = snap.frameSeq;

    // 自适应插值时间：匹配实际快照间隔，避免固定 50ms 导致抖动
    if (m_firstSnapDone) {
        float interpTime = m_timeSinceLastSnap;
        if (interpTime < 0.03f) interpTime = 0.03f;  // 最低 30ms
        if (interpTime > 0.15f) interpTime = 0.15f;  // 最高 150ms（防止大间隙缓慢漂移）
        m_localInterp.setInterpTime(interpTime);
        m_enemyInterp.setInterpTime(interpTime);
    }
    m_timeSinceLastSnap = 0.0f;

    for (int i = 0; i < 2; i++) {
        const TankState& ts = snap.tanks[i];
        bool isLocal = (ts.playerID == m_localPlayerID);
        Tank* tank = isLocal ? m_localTank.get() : m_enemyTank.get();
        StateInterpolator* interp = isLocal ? &m_localInterp : &m_enemyInterp;
        if (!tank) continue;

        // 记录本地坦克的预测误差，用于平滑校正
        if (isLocal && m_firstSnapDone && tank->isAlive()) {
            Vec2 cur = tank->getPos();
            m_predictionError.x = ts.pos.x - cur.x;
            m_predictionError.y = ts.pos.y - cur.y;
        }

        InterpState target = { ts.pos, ts.angle };
        if (!m_firstSnapDone) interp->snap(target);
        else                  interp->setTarget(target);

        tank->setCurHP(ts.curHP);
        tank->setMaxHP(ts.maxHP);
        tank->setAlive(ts.alive);
        tank->setShieldActive(ts.shieldActive);
        tank->setSprintActive(ts.sprintActive);
        tank->setSkillCooldown(ts.skillCooldown);
    }

    m_bullets.clear();
    for (int i = 0; i < snap.bulletCount && i < MAX_BULLETS; i++) {
        const BulletState& bs = snap.bullets[i];
        m_bullets.emplace_back(std::make_unique<Bullet>(bs.pos, bs.vel, bs.owner, bs.damage));
    }

    m_map.applyObstacleDestroyed(snap.obstacleCount, snap.obstaclesDestroyed);
    m_firstSnapDone = true;
}

void GameLoop::onHitReceived(const HitData& hit) {
    // 根据服务端确认的命中事件播放粒子效果
    Tank* victim = nullptr;
    if (m_localTank && m_localTank->getPlayerID() == hit.victimID)
        victim = m_localTank.get();
    else if (m_enemyTank && m_enemyTank->getPlayerID() == hit.victimID)
        victim = m_enemyTank.get();

    if (victim && victim->isAlive()) {
        Vec2 pos = victim->getPos();
        m_particles.emitExplosion(pos, RGB(255, 150, 50));
        m_particles.emitHit(pos, hit.damage);
    }
}

void GameLoop::fireBullet() {
    if (!m_localTank || !m_localTank->canShoot()) return;
    m_localTank->resetShootTimer();

    // 只发送射击请求到服务端，不创建本地子弹
    // 子弹由服务端快照统一管理，避免客户端预创建与服务端位置不一致导致视觉跳变
    if (m_netClient && m_netClient->isConnected()) {
        float a = m_localTank->getAngle();
        Vec2 p = m_localTank->getPos();
        char buf[256]; uint16_t len;
        MsgCodec::encodeShoot(m_localTank->getPlayerID(), p, a, buf, len);
        m_netClient->sendMsg(MsgID::C2S_SHOOT, buf, len);
    }
}

// ====== 渲染（不变）======

void GameLoop::drawMap() {
    setfillcolor(RGB(40,55,40));
    fillrectangle(0,0,MapConfig::WIDTH,MapConfig::HEIGHT);
    setlinecolor(RGB(100,100,100)); setlinestyle(PS_SOLID,2);
    rectangle(0,0,MapConfig::WIDTH-1,MapConfig::HEIGHT-1);
    m_map.draw();
}

void GameLoop::drawTank(Tank* tank, COLORREF body, COLORREF turret) {
    if (!tank || !tank->isAlive()) return;
    int cx=(int)tank->getPos().x, cy=(int)tank->getPos().y;
    float a=tank->getAngle(), ca=std::cos(a), sa=std::sin(a);

    int hw=15,hh=11;
    int lc[4][2]={{-hw,-hh},{hw,-hh},{hw,hh},{-hw,hh}};
    POINT pts[4];
    for(int i=0;i<4;i++){
        pts[i].x=cx+(int)(lc[i][0]*ca-lc[i][1]*sa);
        pts[i].y=cy+(int)(lc[i][0]*sa+lc[i][1]*ca);
    }
    setfillcolor(body); setlinecolor(RGB(30,30,30)); fillpolygon(pts,4);
    setfillcolor(turret); fillcircle(cx,cy,8);
    int bl=18, bx=cx+(int)(ca*bl), by=cy+(int)(sa*bl);
    setlinecolor(turret); setlinestyle(PS_SOLID,4); line(cx,cy,bx,by);

    if(tank->isShieldActive()){ setlinecolor(RGB(100,200,255)); setlinestyle(PS_SOLID,2); circle(cx,cy,22); }
    if(tank->isSprintActive()){ setlinecolor(RGB(255,200,50)); setlinestyle(PS_SOLID,1);
        for(int i=0;i<3;i++){ int tx=cx-(int)(ca*(10+i*6)),ty=cy-(int)(sa*(10+i*6)); circle(tx,ty,3-i); } }
}

void GameLoop::drawBullets() {
    for(auto& b:m_bullets){
        if(!b->isAlive())continue;
        COLORREF c=(b->getOwner()==m_localPlayerID)?RGB(255,255,100):RGB(255,100,100);
        setfillcolor(c); fillcircle((int)b->getPos().x,(int)b->getPos().y,4);
    }
}

void GameLoop::drawHUD() {
    if(m_localTank){
        int x=10,y=10, bw=120,bh=12;
        settextstyle(18,0,"黑体"); settextcolor(WHITE); outtextxy(x,y,"玩家");
        float r=(float)m_localTank->getCurHP()/m_localTank->getMaxHP();
        setfillcolor(RGB(60,60,60)); fillrectangle(x,y+22,x+bw,y+22+bh);
        setfillcolor(r>0.3f?RGB(50,200,50):RGB(200,50,50)); fillrectangle(x,y+22,x+(int)(bw*r),y+22+bh);
        char t[32]; snprintf(t,sizeof(t),"HP:%d/%d",m_localTank->getCurHP(),m_localTank->getMaxHP());
        settextstyle(14,0,"宋体"); settextcolor(WHITE); outtextxy(x+bw+5,y+20,t);
        if(m_localTank->getSkillCooldown()>0){ snprintf(t,sizeof(t),"技能CD:%.1fs",m_localTank->getSkillCooldown()); settextcolor(RGB(200,200,200)); outtextxy(x,y+40,t); }
        else{ settextcolor(RGB(100,255,100)); outtextxy(x,y+40,"技能就绪[F]"); }
    }
    if(m_enemyTank){
        int x=MapConfig::WIDTH-170,y=10,bw=120,bh=12;
        settextstyle(18,0,"黑体"); settextcolor(WHITE); outtextxy(x,y,"敌人");
        float r=(float)m_enemyTank->getCurHP()/m_enemyTank->getMaxHP();
        setfillcolor(RGB(60,60,60)); fillrectangle(x,y+22,x+bw,y+22+bh);
        setfillcolor(r>0.3f?RGB(50,200,50):RGB(200,50,50)); fillrectangle(x,y+22,x+(int)(bw*r),y+22+bh);
        char t[32]; snprintf(t,sizeof(t),"HP:%d/%d",m_enemyTank->getCurHP(),m_enemyTank->getMaxHP());
        settextstyle(14,0,"宋体"); settextcolor(WHITE); outtextxy(x+bw+5,y+20,t);
    }
    settextstyle(14,0,"宋体"); settextcolor(RGB(150,150,150));
    outtextxy(MapConfig::WIDTH/2-80,MapConfig::HEIGHT-25,"WASD移动 空格射击 F技能");
}

void GameLoop::draw() {
    drawMap();
    drawTank(m_localTank.get(), RGB(50,120,200), RGB(80,160,255));
    drawTank(m_enemyTank.get(), RGB(200,60,60), RGB(255,80,80));
    drawBullets();
    m_particles.draw();
    m_map.drawGrassOverlay();
    m_deathAnim.draw();
    drawHUD();
    if(m_localTank&&m_enemyTank)
        Minimap::draw(m_localTank->getPos(),m_enemyTank->getPos(),m_localTank->isAlive(),m_enemyTank->isAlive());
    if(m_gameOver){
        setfillstyle(BS_NULL); settextstyle(48,0,"黑体");
        bool win=m_enemyTank&&!m_enemyTank->isAlive();
        settextcolor(win?RGB(50,255,50):RGB(255,50,50));
        const char* txt=win?"胜  利!":"败  北!";
        int tw=textwidth(txt); outtextxy((MapConfig::WIDTH-tw)/2,MapConfig::HEIGHT/2-30,txt);
        settextstyle(20,0,"宋体"); settextcolor(RGB(200,200,200));
        outtextxy(MapConfig::WIDTH/2-80,MapConfig::HEIGHT/2+30,"按 ESC 返回主菜单");
    }
    FlushBatchDraw();
}
