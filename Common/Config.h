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

namespace CollisionConfig {
    constexpr float TANK_RADIUS   = 20.0f;
    constexpr float BULLET_RADIUS = 5.0f;
}

enum class ObstacleType : uint8_t {
    BRICK = 0,  // 砖墙：可被子弹摧毁
    STEEL = 1,  // 钢墙：不可摧毁
    GRASS = 2,  // 草丛：无碰撞，仅客户端视觉效果
};

struct Obstacle {
    Vec2         pos;
    Vec2         size;   // 半宽半高
    ObstacleType type;
    int          curHP;  // 仅砖墙有效，<=0 表示已摧毁
    int          maxHP;
    bool         destroyed;
};

namespace MapConfig {
    constexpr int WIDTH  = 1200;
    constexpr int HEIGHT = 800;

    constexpr int MAX_OBSTACLES = 64;

    // 默认地图障碍物布局
    inline void GetDefaultObstacles(Obstacle* obstacles, int& count) {
        count = 0;

        auto add = [&](float x, float y, float w, float h, ObstacleType t) {
            if (count >= MAX_OBSTACLES) return;
            obstacles[count].pos = {x, y};
            obstacles[count].size = {w, h};
            obstacles[count].type = t;
            obstacles[count].curHP = (t == ObstacleType::BRICK) ? 50 : 9999;
            obstacles[count].maxHP = obstacles[count].curHP;
            obstacles[count].destroyed = false;
            ++count;
        };

        // 边界钢墙
        add(600, 5,   600, 5,   ObstacleType::STEEL);  // 上
        add(600, 795, 600, 5,   ObstacleType::STEEL);  // 下
        add(5,   400, 5,   400, ObstacleType::STEEL);  // 左
        add(1195, 400, 5,  400, ObstacleType::STEEL);  // 右

        // 中央对称障碍物群
        add(400, 250, 80, 15, ObstacleType::BRICK);
        add(800, 250, 80, 15, ObstacleType::BRICK);
        add(400, 550, 80, 15, ObstacleType::BRICK);
        add(800, 550, 80, 15, ObstacleType::BRICK);

        add(600, 350, 120, 15, ObstacleType::STEEL);
        add(600, 450, 120, 15, ObstacleType::STEEL);

        add(250, 400, 15, 100, ObstacleType::BRICK);
        add(950, 400, 15, 100, ObstacleType::BRICK);

        // 角落砖墙
        add(200, 150, 60, 15, ObstacleType::BRICK);
        add(1000, 150, 60, 15, ObstacleType::BRICK);
        add(200, 650, 60, 15, ObstacleType::BRICK);
        add(1000, 650, 60, 15, ObstacleType::BRICK);
    }
}

namespace NetConfig {
    constexpr int RECV_BUF_SIZE = 4096;
    constexpr int SEND_BUF_SIZE = 4096;
}
