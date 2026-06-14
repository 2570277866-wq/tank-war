#include "GameWorld.h"
#include "../Core/Logger.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void GameWorld::Reset() {
    memset(tanks, 0, sizeof(tanks));
    memset(bullets, 0, sizeof(bullets));
    bulletCount = 0;
    memset(skills, 0, sizeof(skills));
    frameSeq = 0;
    memset(pendingInput, 0, sizeof(pendingInput));
    shootRequested[0] = shootRequested[1] = false;
    skillRequested[0] = skillRequested[1] = false;
    shootTimer[0] = shootTimer[1] = 0.0f;
    playerIDs[0] = playerIDs[1] = -1;
    gameOver = false;
    winnerID = -1;
    hitEventCount = 0;
    winByForfeit = false;
    memset(obstacles, 0, sizeof(obstacles));
    obstacleCount = 0;
    kills[0] = kills[1] = 0;
    inputCountThisTick[0] = inputCountThisTick[1] = 0;
    shootSpamCount[0] = shootSpamCount[1] = 0;
    skillSpamCount[0] = skillSpamCount[1] = 0;
    lastValidPos[0] = {0, 0};
    lastValidPos[1] = {0, 0};
    isInit = false;
}

void GameWorld::Init(int p1ID, TankType t1, int p2ID, TankType t2) {
    Reset();
    isInit = true;
    playerIDs[0] = p1ID;
    playerIDs[1] = p2ID;

    auto getAttrs = [](TankType t) -> const TankConfig::Attrs& {
        switch (t) {
            case TankType::HEAVY: return TankConfig::HEAVY;
            case TankType::LIGHT: return TankConfig::LIGHT;
            case TankType::SCOUT: return TankConfig::SCOUT;
            default: return TankConfig::HEAVY;
        }
    };

    const auto& at1 = getAttrs(t1);
    const auto& at2 = getAttrs(t2);

    tanks[0].pos      = {100.0f, 400.0f};
    tanks[0].angle    = 0.0f;
    tanks[0].curHP    = at1.maxHP;
    tanks[0].maxHP    = at1.maxHP;
    tanks[0].type     = t1;
    tanks[0].playerID = p1ID;
    tanks[0].alive    = true;
    tanks[0].shieldActive = false;
    tanks[0].sprintActive = false;
    tanks[0].shootTimer   = 0.0f;
    tanks[0].skillCooldown = 0.0f;
    tanks[0].skillTimer    = 0.0f;

    tanks[1].pos      = {1100.0f, 400.0f};
    tanks[1].angle    = M_PI;
    tanks[1].curHP    = at2.maxHP;
    tanks[1].maxHP    = at2.maxHP;
    tanks[1].type     = t2;
    tanks[1].playerID = p2ID;
    tanks[1].alive    = true;
    tanks[1].shieldActive = false;
    tanks[1].sprintActive = false;
    tanks[1].shootTimer   = 0.0f;
    tanks[1].skillCooldown = 0.0f;
    tanks[1].skillTimer    = 0.0f;

    skills[0] = {p1ID, SkillType::SHIELD, 0.0f};
    skills[1] = {p2ID, SkillType::SHIELD, 0.0f};
    lastValidPos[0] = tanks[0].pos;
    lastValidPos[1] = tanks[1].pos;

    MapConfig::GetDefaultObstacles(obstacles, obstacleCount);
}

void GameWorld::SetInput(int slot, const InputState& input) {
    std::lock_guard<std::mutex> lock(inputMutex);
    pendingInput[slot] = input;
    inputCountThisTick[slot]++;
}

void GameWorld::RequestShoot(int slot) {
    std::lock_guard<std::mutex> lock(inputMutex);
    shootRequested[slot] = true;
    shootSpamCount[slot]++;
}

void GameWorld::RequestSkill(int slot) {
    std::lock_guard<std::mutex> lock(inputMutex);
    skillRequested[slot] = true;
    skillSpamCount[slot]++;
}

void GameWorld::Tick(float dt) {
    if (!isInit || gameOver) return;

    hitEventCount = 0;

    bool shootReq[2];
    bool skillReq[2];
    {
        std::lock_guard<std::mutex> lock(inputMutex);
        shootReq[0] = shootRequested[0];
        shootReq[1] = shootRequested[1];
        skillReq[0] = skillRequested[0];
        skillReq[1] = skillRequested[1];
        shootRequested[0] = shootRequested[1] = false;
        skillRequested[0] = skillRequested[1] = false;
    }

    for (int i = 0; i < 2; ++i) {
        if (!tanks[i].alive) continue;

        MoveTank(i, dt);

        if (shootTimer[i] > 0.0f) shootTimer[i] -= dt;
        if (tanks[i].skillCooldown > 0.0f) tanks[i].skillCooldown -= dt;
        if (tanks[i].skillTimer > 0.0f) tanks[i].skillTimer -= dt;
    }

    DeactivateExpiredSkills(dt);

    for (int i = 0; i < 2; ++i) {
        if (!tanks[i].alive) continue;

        if (shootReq[i] && shootTimer[i] <= 0.0f) {
            SpawnBullet(i);
            shootTimer[i] = SHOOT_COOLDOWN;
        }

        if (skillReq[i] && tanks[i].skillCooldown <= 0.0f) {
            const auto& attr = GetAttrs(i);
            SkillType st;
            switch (tanks[i].type) {
                case TankType::HEAVY: st = SkillType::SHIELD; break;
                case TankType::LIGHT: st = SkillType::SPRINT; break;
                case TankType::SCOUT: st = SkillType::SCATTER; break;
                default: st = SkillType::SHIELD; break;
            }

            if (st == SkillType::SHIELD) {
                tanks[i].shieldActive = true;
                tanks[i].skillTimer = attr.skillDuration;
                tanks[i].skillCooldown = attr.skillCooldown;
                skills[i] = {tanks[i].playerID, SkillType::SHIELD, attr.skillDuration};
            } else if (st == SkillType::SPRINT) {
                tanks[i].sprintActive = true;
                tanks[i].skillTimer = attr.skillDuration;
                tanks[i].skillCooldown = attr.skillCooldown;
                skills[i] = {tanks[i].playerID, SkillType::SPRINT, attr.skillDuration};
            } else if (st == SkillType::SCATTER) {
                SpawnScatter(i);
                tanks[i].skillCooldown = attr.skillCooldown;
                skills[i] = {tanks[i].playerID, SkillType::SCATTER, 0.0f};
            }
        }
    }

    MoveBullets(dt);
    CheckBulletBounds();
    CheckBulletObstacleCollisions();

    for (int i = 0; i < 2; ++i) {
        if (tanks[i].alive) {
            int other = 1 - i;
            if (tanks[other].alive) {
                CheckBulletTankCollisions(i);
            }
        }
    }

    for (int i = 0; i < 2; ++i) {
        if (!tanks[i].alive) {
            int other = 1 - i;
            gameOver = true;
            winnerID = tanks[other].playerID;
            winByForfeit = false;
            break;
        }
    }

    // 反作弊校验
    for (int i = 0; i < 2; ++i) {
        if (!tanks[i].alive) continue;
        const auto& attr = GetAttrs(i);

        // 速度上限检查：防止位置瞬移
        float maxSpeed = attr.speed * (tanks[i].sprintActive ? 2.0f : 1.0f);
        float maxDist = maxSpeed * dt + 1.0f; // 1.0f tolerance
        float actualDist = std::sqrt(
            (tanks[i].pos.x - lastValidPos[i].x) * (tanks[i].pos.x - lastValidPos[i].x) +
            (tanks[i].pos.y - lastValidPos[i].y) * (tanks[i].pos.y - lastValidPos[i].y)
        );
        if (actualDist > maxDist) {
            // 瞬移作弊，回退到上一合法位置
            tanks[i].pos = lastValidPos[i];
        }

        // 输入频率异常告警（正常客户端 30-60fps 发送 ~1.5-3 条/tick）
        if (inputCountThisTick[i] > 5) {
            Logger::Get().Cheat("Player " + std::to_string(tanks[i].playerID) +
                                " sent " + std::to_string(inputCountThisTick[i]) +
                                " inputs in one tick");
        }

        // 射击频率异常
        if (shootSpamCount[i] > 3) {
            Logger::Get().Cheat("Player " + std::to_string(tanks[i].playerID) +
                                " attempted " + std::to_string(shootSpamCount[i]) +
                                " shots in one tick");
        }

        // 保存合法位置
        lastValidPos[i] = tanks[i].pos;
        inputCountThisTick[i] = 0;
        shootSpamCount[i] = 0;
        skillSpamCount[i] = 0;
    }

    ++frameSeq;
}

void GameWorld::MoveTank(int slot, float dt) {
    const auto& attr = GetAttrs(slot);
    const InputState& input = pendingInput[slot];

    float speed = attr.speed;
    if (tanks[slot].sprintActive) speed *= 2.0f;

    if (input.a) tanks[slot].angle -= attr.rotateSpeed * dt;
    if (input.d) tanks[slot].angle += attr.rotateSpeed * dt;

    Vec2 dir = ForwardVec(slot);
    if (input.w) {
        tanks[slot].pos.x += dir.x * speed * dt;
        tanks[slot].pos.y += dir.y * speed * dt;
    }
    if (input.s) {
        tanks[slot].pos.x -= dir.x * speed * dt;
        tanks[slot].pos.y -= dir.y * speed * dt;
    }

    Judge::ClampToBounds(tanks[slot].pos, CollisionConfig::TANK_RADIUS);

    // 障碍物碰撞检测与推离
    for (int i = 0; i < obstacleCount; ++i) {
        if (obstacles[i].destroyed) continue;
        if (obstacles[i].type == ObstacleType::GRASS) continue;
        if (Judge::CircleRect(tanks[slot].pos, CollisionConfig::TANK_RADIUS,
                              obstacles[i].pos, obstacles[i].size)) {
            Judge::PushCircleOutOfRect(tanks[slot].pos, CollisionConfig::TANK_RADIUS,
                                       obstacles[i].pos, obstacles[i].size);
        }
    }

    // 坦克-坦克碰撞：防止两辆坦克重叠（画面表现为坐标错位）
    int other = 1 - slot;
    if (tanks[other].alive) {
        float dx = tanks[slot].pos.x - tanks[other].pos.x;
        float dy = tanks[slot].pos.y - tanks[other].pos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        float minDist = CollisionConfig::TANK_RADIUS * 2.0f;
        if (dist < minDist && dist > 0.0001f) {
            float overlap = minDist - dist;
            // 各推一半，避免单次过度分离
            float pushX = (dx / dist) * overlap * 0.5f;
            float pushY = (dy / dist) * overlap * 0.5f;
            tanks[slot].pos.x += pushX;
            tanks[slot].pos.y += pushY;
            tanks[other].pos.x -= pushX;
            tanks[other].pos.y -= pushY;
        }
    }

    // 障碍物推离后重新钳制边界
    Judge::ClampToBounds(tanks[slot].pos, CollisionConfig::TANK_RADIUS);
    // 对方坦克也可能被推出边界，同样钳制
    if (tanks[other].alive) {
        Judge::ClampToBounds(tanks[other].pos, CollisionConfig::TANK_RADIUS);
    }
}

void GameWorld::SpawnBullet(int slot) {
    if (bulletCount >= MAX_BULLETS) return;

    const auto& attrs = GetAttrs(slot);
    Vec2 dir = ForwardVec(slot);
    float spawnDist = CollisionConfig::TANK_RADIUS + CollisionConfig::BULLET_RADIUS + 1.0f;

    bullets[bulletCount].pos = {
        tanks[slot].pos.x + dir.x * spawnDist,
        tanks[slot].pos.y + dir.y * spawnDist
    };
    bullets[bulletCount].vel = {dir.x * attrs.bulletSpeed, dir.y * attrs.bulletSpeed};
    bullets[bulletCount].owner = tanks[slot].playerID;
    bullets[bulletCount].damage = attrs.damage;
    bullets[bulletCount].type = BulletType::NORMAL;
    ++bulletCount;
}

void GameWorld::SpawnScatter(int slot) {
    if (bulletCount + 3 > MAX_BULLETS) return;

    const auto& attrs = GetAttrs(slot);
    float baseAngle = tanks[slot].angle;
    float spread = 15.0f * M_PI / 180.0f;
    float angles[3] = {baseAngle - spread, baseAngle, baseAngle + spread};
    float spawnDist = CollisionConfig::TANK_RADIUS + CollisionConfig::BULLET_RADIUS + 1.0f;

    for (int i = 0; i < 3; ++i) {
        float ax = std::cos(angles[i]);
        float ay = std::sin(angles[i]);
        bullets[bulletCount].pos = {
            tanks[slot].pos.x + ax * spawnDist,
            tanks[slot].pos.y + ay * spawnDist
        };
        bullets[bulletCount].vel = {ax * attrs.bulletSpeed, ay * attrs.bulletSpeed};
        bullets[bulletCount].owner = tanks[slot].playerID;
        bullets[bulletCount].damage = attrs.damage;
        bullets[bulletCount].type = BulletType::SCATTER;
        ++bulletCount;
    }
}

void GameWorld::MoveBullets(float dt) {
    for (int i = 0; i < bulletCount; ++i) {
        bullets[i].pos.x += bullets[i].vel.x * dt;
        bullets[i].pos.y += bullets[i].vel.y * dt;
    }
}

void GameWorld::CheckBulletTankCollisions(int bulletOwnerSlot) {
    int other = 1 - bulletOwnerSlot;

    for (int i = bulletCount - 1; i >= 0; --i) {
        if (bullets[i].owner == tanks[other].playerID) continue; // skip own bullets hitting self scenario (shouldn't happen)

        if (Judge::CircleCircle(bullets[i].pos, CollisionConfig::BULLET_RADIUS,
                                tanks[other].pos, CollisionConfig::TANK_RADIUS)) {
            if (tanks[other].alive) {
                ApplyDamage(other, bullets[i].damage, bulletOwnerSlot);
            }
            RemoveBullet(i);
        }
    }
}

void GameWorld::RemoveBullet(int index) {
    if (index < bulletCount - 1) {
        bullets[index] = bullets[bulletCount - 1];
    }
    --bulletCount;
}

void GameWorld::CheckBulletBounds() {
    for (int i = bulletCount - 1; i >= 0; --i) {
        if (!Judge::InBounds(bullets[i].pos, CollisionConfig::BULLET_RADIUS)) {
            RemoveBullet(i);
        }
    }
}

void GameWorld::CheckBulletObstacleCollisions() {
    for (int i = bulletCount - 1; i >= 0; --i) {
        for (int j = 0; j < obstacleCount; ++j) {
            if (obstacles[j].destroyed) continue;
            if (obstacles[j].type == ObstacleType::GRASS) continue;

            if (Judge::CircleRect(bullets[i].pos, CollisionConfig::BULLET_RADIUS,
                                  obstacles[j].pos, obstacles[j].size)) {
                if (obstacles[j].type == ObstacleType::BRICK) {
                    obstacles[j].curHP -= bullets[i].damage;
                    if (obstacles[j].curHP <= 0) {
                        obstacles[j].curHP = 0;
                        obstacles[j].destroyed = true;
                    }
                }
                RemoveBullet(i);
                break; // bullet destroyed, skip remaining obstacles
            }
        }
    }
}

void GameWorld::DeactivateExpiredSkills(float /*dt*/) {
    for (int i = 0; i < 2; ++i) {
        if (tanks[i].shieldActive && tanks[i].skillTimer <= 0.0f) {
            tanks[i].shieldActive = false;
            skills[i].timer = 0.0f;
        }
        if (tanks[i].sprintActive && tanks[i].skillTimer <= 0.0f) {
            tanks[i].sprintActive = false;
            skills[i].timer = 0.0f;
        }
    }
}

void GameWorld::ApplyDamage(int victimSlot, int damage, int attackerSlot) {
    if (tanks[victimSlot].shieldActive) damage = 0;

    tanks[victimSlot].curHP -= damage;
    if (tanks[victimSlot].curHP < 0) tanks[victimSlot].curHP = 0;

    if (hitEventCount < MAX_BULLETS) {
        hitEvents[hitEventCount++] = {
            victimSlot,
            damage,
            tanks[victimSlot].curHP,
            attackerSlot
        };
    }

    if (tanks[victimSlot].curHP <= 0) {
        tanks[victimSlot].alive = false;
        kills[attackerSlot]++;
    }
}

Vec2 GameWorld::ForwardVec(int slot) const {
    return {std::cos(tanks[slot].angle), std::sin(tanks[slot].angle)};
}

const TankConfig::Attrs& GameWorld::GetAttrs(int slot) const {
    switch (tanks[slot].type) {
        case TankType::HEAVY: return TankConfig::HEAVY;
        case TankType::LIGHT: return TankConfig::LIGHT;
        case TankType::SCOUT: return TankConfig::SCOUT;
        default: return TankConfig::HEAVY;
    }
}

Snapshot GameWorld::PackSnapshot() const {
    Snapshot snap;
    snap.frameSeq = frameSeq;
    snap.tanks[0] = tanks[0];
    snap.tanks[1] = tanks[1];
    snap.bulletCount = bulletCount;
    for (int i = 0; i < bulletCount && i < MAX_BULLETS; ++i) {
        snap.bullets[i] = bullets[i];
    }
    snap.skills[0] = skills[0];
    snap.skills[1] = skills[1];
    snap.obstacleCount = obstacleCount;
    for (int i = 0; i < obstacleCount && i < 64; ++i) {
        snap.obstaclesDestroyed[i] = obstacles[i].destroyed;
    }
    return snap;
}
