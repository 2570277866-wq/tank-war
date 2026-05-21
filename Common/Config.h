#pragma once

#include "Protocol.h"

namespace TankConfig {
    struct Attrs {
        int   maxHP;
        float speed;
        float rotateSpeed;
        int   damage;
        float bulletSpeed;
        float skillCooldown;
        float skillDuration;
    };

    constexpr Attrs HEAVY = {
        .maxHP        = 200,
        .speed        = 1.5f,
        .rotateSpeed  = 1.5f,
        .damage       = 30,
        .bulletSpeed  = 5.0f,
        .skillCooldown = 15.0f,
        .skillDuration = 3.0f,
    };

    constexpr Attrs LIGHT = {
        .maxHP        = 150,
        .speed        = 2.5f,
        .rotateSpeed  = 2.5f,
        .damage       = 20,
        .bulletSpeed  = 7.0f,
        .skillCooldown = 12.0f,
        .skillDuration = 2.0f,
    };

    constexpr Attrs SCOUT = {
        .maxHP        = 100,
        .speed        = 3.5f,
        .rotateSpeed  = 3.5f,
        .damage       = 15,
        .bulletSpeed  = 9.0f,
        .skillCooldown = 10.0f,
        .skillDuration = 0.0f,
    };
}

namespace MapConfig {
    constexpr int WIDTH  = 1200;
    constexpr int HEIGHT = 800;
}

namespace CollisionConfig {
    constexpr float TANK_RADIUS   = 20.0f;
    constexpr float BULLET_RADIUS = 5.0f;
}

namespace NetConfig {
    constexpr int RECV_BUF_SIZE = 4096;
    constexpr int SEND_BUF_SIZE = 4096;
}
