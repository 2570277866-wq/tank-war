#include "GameLoop.h"
#include <iostream>
#include <cmath>
#include <algorithm>

constexpr float PI = 3.14159265358979f;

GameLoop::GameLoop() {}
GameLoop::~GameLoop() { stop(); }

void GameLoop::init() {
    m_bullets.clear();
    m_particles.clear();
    m_deathAnim = DeathAnimator{};  // 重置死亡动画
    m_gameOver = false;
    m_running = false;
    m_prevSpace = m_prevF = false;
    m_firstSnapDone = false;
    m_lastSnapSeq = 0;
    m_timeSinceLastSnap = 0.0f;
    m_totalDamage = 0;
    m_kills = 0;
    m_gameTime = 0.0f;
    m_prevLocalSkillTimer = m_prevEnemySkillTimer = 0.0f;
    m_prevLocalShield = m_prevEnemyShield = false;
    m_prevLocalSprint = m_prevEnemySprint = false;
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
    m_particles.clear();
    m_deathAnim = DeathAnimator{};
    m_gameOver = false;
    m_running = false;
    m_prevSpace = m_prevF = false;
    m_firstSnapDone = false;
    m_lastSnapSeq = 0;
    m_totalDamage = 0;
    m_kills = 0;
    m_gameTime = 0.0f;
    m_prevLocalSkillTimer = m_prevEnemySkillTimer = 0.0f;
    m_prevLocalShield = m_prevEnemyShield = false;
    m_prevLocalSprint = m_prevEnemySprint = false;
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
            // 插值后的位置也做碰撞检测，防止快照间线性插值穿过障碍物
            m_map.checkTankCollision(s.pos, CollisionConfig::TANK_RADIUS);
            m_localTank->setPos(s.pos);
            m_localTank->setAngle(s.angle);
        }
        if (m_enemyTank) {
            InterpState s = m_enemyInterp.getCurrent();
            m_map.checkTankCollision(s.pos, CollisionConfig::TANK_RADIUS);
            m_enemyTank->setPos(s.pos);
            m_enemyTank->setAngle(s.angle);
        }
    }

    // 本地角度预测：转向立即响应，不影响位置同步
    if (m_localTank) {
        float r = m_localTank->getType() == TankType::HEAVY ? TankConfig::HEAVY.rotateSpeed :
                 m_localTank->getType() == TankType::LIGHT ? TankConfig::LIGHT.rotateSpeed :
                 TankConfig::SCOUT.rotateSpeed;
        if (input.a) m_localTank->setAngle(m_localTank->getAngle() - r * dt);
        if (input.d) m_localTank->setAngle(m_localTank->getAngle() + r * dt);
    }

    // 注意：不做客户端位置预测。
    // 所有坦克位置完全由服务器快照 + 插值器驱动，
    // 保证双方玩家看到的坦克-障碍物相对位置完全一致。

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
        // 技能激活本地粒子预览（避免等服务端快照的延迟）
        SkillType sk = m_localTank->getSkillType();
        if (sk == SkillType::SHIELD) {
            m_particles.emitShieldActivate(m_localTank->getPos());
        } else if (sk == SkillType::SPRINT) {
            m_particles.emitSprintActivate(m_localTank->getPos());
        } else if (sk == SkillType::SCATTER) {
            // 散射：枪口闪光
            float a = m_localTank->getAngle();
            Vec2 p = m_localTank->getPos();
            float muzzleDist = CollisionConfig::TANK_RADIUS + 8.0f;
            Vec2 muzzlePos = { p.x + std::cos(a) * muzzleDist,
                               p.y + std::sin(a) * muzzleDist };
            m_particles.emitScatterMuzzle(muzzlePos, a);
        }
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
    for (auto& b : m_bullets) {
        b->update(dt);
        // 散射子弹留下拖尾粒子
        if (b->getType() == BulletType::SCATTER && b->isAlive()) {
            bool isLocal = (b->getOwner() == m_localPlayerID);
            COLORREF trailColor = isLocal ? RGB(255, 200, 50) : RGB(255, 140, 30);
            m_particles.emitBulletTrail(b->getPos(), trailColor, 2.5f);
        }
    }
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(),
        [](auto& b) { return !b->isAlive(); }), m_bullets.end());
}

void GameLoop::checkCollisions() {
    // 注意：子弹-坦克碰撞由服务端权威判定，客户端不做此检测
    // 客户端只处理子弹-障碍物碰撞（纯视觉效果）
    for (auto& bullet : m_bullets) {
        if (!bullet->isAlive()) continue;
        if (m_map.checkBulletCollision(bullet->getPos(), CollisionConfig::BULLET_RADIUS, bullet->getDamage())) {
            // 根据子弹类型选择爆炸颜色
            COLORREF expColor = (bullet->getType() == BulletType::SCATTER)
                ? RGB(255, 180, 50)   // 散射子弹：橙金色爆炸
                : RGB(200, 150, 50);   // 普通子弹：暗金色爆炸
            int particleCount = (bullet->getType() == BulletType::SCATTER) ? 10 : 6;
            m_particles.emitExplosion(bullet->getPos(), expColor, particleCount);
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
        m_bullets.emplace_back(std::make_unique<Bullet>(bs.pos, bs.vel, bs.owner, bs.damage, bs.type));
    }

    // ===== 技能激活检测：状态从 0→active 时播放一次性特效 =====
    if (m_localTank) {
        if (!m_prevLocalShield && m_localTank->isShieldActive()) {
            m_particles.emitShieldActivate(m_localTank->getPos());
        }
        if (m_prevLocalShield && !m_localTank->isShieldActive()) {
            m_particles.emitShieldDeactivate(m_localTank->getPos());
        }
        if (!m_prevLocalSprint && m_localTank->isSprintActive()) {
            m_particles.emitSprintActivate(m_localTank->getPos());
        }
        m_prevLocalShield = m_localTank->isShieldActive();
        m_prevLocalSprint = m_localTank->isSprintActive();
    }
    if (m_enemyTank) {
        if (!m_prevEnemyShield && m_enemyTank->isShieldActive()) {
            m_particles.emitShieldActivate(m_enemyTank->getPos());
        }
        if (m_prevEnemyShield && !m_enemyTank->isShieldActive()) {
            m_particles.emitShieldDeactivate(m_enemyTank->getPos());
        }
        if (!m_prevEnemySprint && m_enemyTank->isSprintActive()) {
            m_particles.emitSprintActivate(m_enemyTank->getPos());
        }
        m_prevEnemyShield = m_enemyTank->isShieldActive();
        m_prevEnemySprint = m_enemyTank->isSprintActive();
    }

    m_map.applyObstacleDestroyed(snap.obstacleCount, snap.obstaclesDestroyed);
    m_firstSnapDone = true;
}

void GameLoop::onHitReceived(const HitData& hit) {
    // 统计伤害（本地玩家造成的伤害）
    if (m_localTank && hit.attackerID == m_localTank->getPlayerID()) {
        m_totalDamage += hit.damage;
    }

    // 统计击杀（敌人被击杀）
    if (m_enemyTank && hit.victimID == m_enemyTank->getPlayerID() && hit.remainingHP <= 0) {
        m_kills++;
    }

    // 根据服务端确认的命中事件播放粒子效果
    Tank* victim = nullptr;
    if (m_localTank && m_localTank->getPlayerID() == hit.victimID)
        victim = m_localTank.get();
    else if (m_enemyTank && m_enemyTank->getPlayerID() == hit.victimID)
        victim = m_enemyTank.get();

    if (victim && victim->isAlive()) {
        Vec2 pos = victim->getPos();
        // 爆炸粒子数量根据伤害值缩放
        int particleCount = 8 + hit.damage / 5;
        COLORREF expColor = (hit.damage >= 30) ? RGB(255, 100, 30) :  // 高伤害：深橙红
                            (hit.damage >= 20) ? RGB(255, 160, 50) :   // 中伤害：橙色
                                                 RGB(255, 200, 80);    // 低伤害：黄橙
        m_particles.emitExplosion(pos, expColor, particleCount);
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
    setfillstyle(BS_SOLID);
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

    // ==== 冲刺残影（在坦克本体之前绘制） ====
    if (tank->isSprintActive()) {
        for (int ghost = 1; ghost <= 3; ghost++) {
            float alpha = 0.3f - ghost * 0.08f;
            int gx = cx - (int)(ca * ghost * 8);
            int gy = cy - (int)(sa * ghost * 8);
            int hw=15, hh=11;
            int lc[4][2]={{-hw,-hh},{hw,-hh},{hw,hh},{-hw,hh}};
            POINT gp[4];
            for (int i=0;i<4;i++) {
                gp[i].x=gx+(int)(lc[i][0]*ca-lc[i][1]*sa);
                gp[i].y=gy+(int)(lc[i][0]*sa+lc[i][1]*ca);
            }
            int br=(int)(GetRValue(body)*alpha), bg=(int)(GetGValue(body)*alpha), bb=(int)(GetBValue(body)*alpha);
            setfillcolor(RGB(br, bg, bb));
            setlinecolor(RGB((int)(30*alpha), (int)(30*alpha), (int)(30*alpha)));
            fillpolygon(gp, 4);
        }
    }

    // ==== 坦克本体 ====
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

    // ==== 护盾效果：六边形旋转光盾 ====
    if (tank->isShieldActive()) {
        // 外圈脉冲光环
        float pulse = 0.7f + 0.3f * std::sin((float)GetTickCount() * 0.005f);
        int alpha = (int)(180 * pulse);
        setlinecolor(RGB(alpha/2, alpha, alpha));
        setlinestyle(PS_SOLID, 3);
        circle(cx, cy, 24);

        // 六边形护盾
        POINT hex[6];
        float hexR = 22.0f;
        for (int i = 0; i < 6; i++) {
            float ha = a + (3.14159265f * 2.0f * i) / 6.0f;
            hex[i].x = cx + (int)(std::cos(ha) * hexR);
            hex[i].y = cy + (int)(std::sin(ha) * hexR);
        }
        setlinecolor(RGB(60, 180 + (int)(75 * pulse), 255));
        setlinestyle(PS_SOLID, 2);
        polygon(hex, 6);

        // 内圈闪烁
        int innerAlpha = (int)(120 * pulse);
        setlinecolor(RGB(innerAlpha/2, innerAlpha, innerAlpha));
        setlinestyle(PS_DOT, 1);
        circle(cx, cy, 19);
        setlinestyle(PS_SOLID, 1);
    }

    // ==== 冲刺尾焰 ====
    if (tank->isSprintActive()) {
        for (int i = 0; i < 4; i++) {
            int tx = cx - (int)(ca * (12 + i * 7));
            int ty = cy - (int)(sa * (12 + i * 7));
            int flameAlpha = 180 - i * 30;
            int fr = flameAlpha, fg = flameAlpha * 3 / 4, fb = flameAlpha / 4;
            setfillcolor(RGB(fr, fg > 255 ? 255 : fg, fb));
            fillcircle(tx, ty, 4 - i);
        }
    }
}

void GameLoop::drawBullets() {
    for (auto& b : m_bullets) {
        if (!b->isAlive()) continue;

        bool isLocal = (b->getOwner() == m_localPlayerID);
        bool isScatter = (b->getType() == BulletType::SCATTER);
        int dmg = b->getDamage();

        // 子弹大小根据伤害值缩放（最小3，最大6）
        int radius = 3 + dmg / 10;
        if (radius > 6) radius = 6;

        // ==== 散射子弹拖尾（预存位置） ====
        if (isScatter) {
            const auto& trail = b->getTrail();
            int tsz = (int)trail.size();
            for (int i = 0; i < tsz; i++) {
                float alpha = (float)(i + 1) / (tsz + 1);
                int tr = (int)(GetRValue(isLocal ? RGB(255, 200, 50) : RGB(255, 140, 30)) * alpha * 0.5f);
                int tg = (int)(GetGValue(isLocal ? RGB(255, 200, 50) : RGB(255, 140, 30)) * alpha * 0.5f);
                int tb = (int)(GetBValue(isLocal ? RGB(255, 200, 50) : RGB(255, 140, 30)) * alpha * 0.5f);
                setfillcolor(RGB(tr, tg, tb));
                fillcircle((int)trail[i].x, (int)trail[i].y, (int)(radius * 0.6f));
            }
        }

        // ==== 子弹主体渲染 ====
        if (isScatter) {
            // 散射子弹：橙金色菱形效果
            COLORREF outer = isLocal ? RGB(255, 180, 30) : RGB(255, 120, 20);
            COLORREF inner = isLocal ? RGB(255, 240, 100) : RGB(255, 200, 80);

            // 外圈光晕
            setfillcolor(outer);
            fillcircle((int)b->getPos().x, (int)b->getPos().y, radius + 2);

            // 内核
            setfillcolor(inner);
            fillcircle((int)b->getPos().x, (int)b->getPos().y, radius - 1);

            // 菱形高亮
            setfillcolor(RGB(255, 255, 220));
            int cx = (int)b->getPos().x, cy = (int)b->getPos().y;
            int hs = radius / 2 + 1;
            POINT diamond[4] = {
                {cx, cy - hs},
                {cx + hs, cy},
                {cx, cy + hs},
                {cx - hs, cy}
            };
            solidpolygon(diamond, 4);
        } else {
            // 普通子弹：小光点 + 光晕
            COLORREF outer, inner, core;
            if (isLocal) {
                outer = RGB(255, 255, 80);
                inner = RGB(255, 255, 180);
                core  = RGB(255, 255, 255);
            } else {
                outer = RGB(255, 80, 60);
                inner = RGB(255, 130, 110);
                core  = RGB(255, 200, 180);
            }

            // 外圈光晕
            setfillcolor(outer);
            fillcircle((int)b->getPos().x, (int)b->getPos().y, radius);

            // 内核
            setfillcolor(inner);
            fillcircle((int)b->getPos().x, (int)b->getPos().y, radius - 1);

            // 中心高亮
            setfillcolor(core);
            fillcircle((int)b->getPos().x, (int)b->getPos().y, (radius > 3 ? 2 : 1));
        }
    }
}

void GameLoop::drawHUD() {
    auto getSkillName = [](SkillType st) -> const char* {
        switch (st) {
            case SkillType::SHIELD:  return "护盾";
            case SkillType::SPRINT:  return "冲刺";
            case SkillType::SCATTER: return "散射";
            default: return "未知";
        }
    };

    if (m_localTank) {
        int x = 10, y = 10, bw = 120, bh = 12;
        // 玩家标签
        settextstyle(18, 0, "黑体"); settextcolor(WHITE);
        outtextxy(x, y, "玩家");

        // HP条
        float r = (float)m_localTank->getCurHP() / m_localTank->getMaxHP();
        setfillcolor(RGB(60, 60, 60));
        fillrectangle(x, y + 22, x + bw, y + 22 + bh);
        COLORREF hpColor = r > 0.5f ? RGB(50, 200, 50) :
                           r > 0.25f ? RGB(220, 180, 30) : RGB(200, 50, 50);
        setfillcolor(hpColor);
        fillrectangle(x, y + 22, x + (int)(bw * r), y + 22 + bh);
        char t[64];
        snprintf(t, sizeof(t), "HP:%d/%d", m_localTank->getCurHP(), m_localTank->getMaxHP());
        settextstyle(14, 0, "宋体"); settextcolor(WHITE);
        outtextxy(x + bw + 5, y + 20, t);

        // 技能信息
        SkillType st = m_localTank->getSkillType();
        const char* skillName = getSkillName(st);
        float cd = m_localTank->getSkillCooldown();
        bool active = m_localTank->isShieldActive() || m_localTank->isSprintActive();

        if (active) {
            // 技能激活中 - 显示技能名和剩余时间
            snprintf(t, sizeof(t), "%s激活中 %.1fs", skillName,
                     m_localTank->getSkillCooldown() > 0 ? m_localTank->getSkillCooldown() : 0.0f);
            settextcolor(RGB(100, 255, 150));
            outtextxy(x, y + 40, t);

            // 持续时间进度条
            float durPct = 1.0f; // 简化：用技能CD近似
            setfillcolor(RGB(40, 40, 40));
            fillrectangle(x, y + 56, x + bw, y + 56 + 6);
            COLORREF skillBarColor;
            switch (st) {
                case SkillType::SHIELD:  skillBarColor = RGB(80, 180, 255); break;
                case SkillType::SPRINT:  skillBarColor = RGB(255, 200, 50); break;
                case SkillType::SCATTER: skillBarColor = RGB(255, 150, 40); break;
                default: skillBarColor = RGB(150, 150, 150); break;
            }
            setfillcolor(skillBarColor);
            fillrectangle(x, y + 56, x + (int)(bw * durPct), y + 56 + 6);
        } else if (cd > 0) {
            // CD中
            snprintf(t, sizeof(t), "%s CD:%.1fs", skillName, cd);
            settextcolor(RGB(200, 180, 100));
            outtextxy(x, y + 40, t);
        } else {
            // 就绪
            snprintf(t, sizeof(t), "%s 就绪 [F]", skillName);
            settextcolor(RGB(100, 255, 100));
            outtextxy(x, y + 40, t);
        }

        // 坦克类型标签
        const char* tankTypeName = "";
        switch (m_localTank->getType()) {
            case TankType::HEAVY: tankTypeName = "重型坦克"; break;
            case TankType::LIGHT: tankTypeName = "轻型坦克"; break;
            case TankType::SCOUT: tankTypeName = "侦察坦克"; break;
        }
        settextcolor(RGB(180, 180, 180));
        settextstyle(12, 0, "宋体");
        outtextxy(x, y + 68, tankTypeName);

        // 本局统计
        snprintf(t, sizeof(t), "伤害:%d  击杀:%d", m_totalDamage, m_kills);
        outtextxy(x, y + 84, t);
    }

    if (m_enemyTank) {
        int x = MapConfig::WIDTH - 170, y = 10, bw = 120, bh = 12;
        settextstyle(18, 0, "黑体"); settextcolor(WHITE);
        outtextxy(x, y, "敌人");

        float r = (float)m_enemyTank->getCurHP() / m_enemyTank->getMaxHP();
        setfillcolor(RGB(60, 60, 60));
        fillrectangle(x, y + 22, x + bw, y + 22 + bh);
        COLORREF hpColor = r > 0.5f ? RGB(50, 200, 50) :
                           r > 0.25f ? RGB(220, 180, 30) : RGB(200, 50, 50);
        setfillcolor(hpColor);
        fillrectangle(x, y + 22, x + (int)(bw * r), y + 22 + bh);
        char t[64];
        snprintf(t, sizeof(t), "HP:%d/%d", m_enemyTank->getCurHP(), m_enemyTank->getMaxHP());
        settextstyle(14, 0, "宋体"); settextcolor(WHITE);
        outtextxy(x + bw + 5, y + 20, t);

        // 敌人技能指示
        if (m_enemyTank->isShieldActive()) {
            settextcolor(RGB(100, 200, 255));
            outtextxy(x, y + 40, "护盾激活中");
        } else if (m_enemyTank->isSprintActive()) {
            settextcolor(RGB(255, 200, 50));
            outtextxy(x, y + 40, "冲刺激活中");
        }

        // 敌人坦克类型
        const char* enemyTypeName = "";
        switch (m_enemyTank->getType()) {
            case TankType::HEAVY: enemyTypeName = "重型坦克"; break;
            case TankType::LIGHT: enemyTypeName = "轻型坦克"; break;
            case TankType::SCOUT: enemyTypeName = "侦察坦克"; break;
        }
        settextcolor(RGB(180, 180, 180));
        settextstyle(12, 0, "宋体");
        outtextxy(x, y + 54, enemyTypeName);
    }

    // 底部操作提示
    settextstyle(14, 0, "宋体");
    settextcolor(RGB(150, 150, 150));
    outtextxy(MapConfig::WIDTH / 2 - 100, MapConfig::HEIGHT - 25,
              "WASD移动  空格射击  F技能");
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
        settextstyle(48,0,"黑体");
        bool win=m_enemyTank&&!m_enemyTank->isAlive();
        settextcolor(win?RGB(50,255,50):RGB(255,50,50));
        const char* txt=win?"胜  利!":"败  北!";
        int tw=textwidth(txt); outtextxy((MapConfig::WIDTH-tw)/2,MapConfig::HEIGHT/2-30,txt);
        settextstyle(20,0,"宋体"); settextcolor(RGB(200,200,200));
        outtextxy(MapConfig::WIDTH/2-80,MapConfig::HEIGHT/2+30,"按 ESC 返回主菜单");
    }
    FlushBatchDraw();
}
