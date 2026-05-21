#pragma once

#include <mutex>
#include <cstring>
#include <cmath>
#include "../../Common/Protocol.h"
#include "../../Common/Config.h"
#include "Judge.h"

struct HitEvent {
    int victimSlot;
    int damage;
    int remainingHP;
    int attackerSlot;
};

class GameWorld {
public:
    TankState        tanks[2];
    BulletState      bullets[MAX_BULLETS];
    int              bulletCount = 0;
    SkillEffectState skills[2];
    uint32_t         frameSeq = 0;

    InputState       pendingInput[2];
    bool             shootRequested[2] = {false, false};
    bool             skillRequested[2] = {false, false};
    std::mutex       inputMutex;

    float            shootTimer[2] = {0.0f, 0.0f};
    int              playerIDs[2] = {-1, -1};
    bool             gameOver = false;
    int              winnerID = -1;

    HitEvent         hitEvents[MAX_BULLETS];
    int              hitEventCount = 0;
    bool             winByForfeit = false;

    void Reset();
    void Init(int p1ID, TankType t1, int p2ID, TankType t2);
    void SetInput(int slot, const InputState& input);
    void RequestShoot(int slot);
    void RequestSkill(int slot);
    void Tick(float dt);
    Snapshot PackSnapshot() const;

private:
    bool isInit = false;

    void MoveTank(int slot, float dt);
    void SpawnBullet(int slot);
    void SpawnScatter(int slot);
    void MoveBullets(float dt);
    void CheckBulletTankCollisions(int slot);
    void RemoveBullet(int index);
    void CheckBulletBounds();
    void DeactivateExpiredSkills(float dt);

    Vec2 ForwardVec(int slot) const;
    void ApplyDamage(int victimSlot, int damage, int attackerSlot);
    const TankConfig::Attrs& GetAttrs(int slot) const;
};
