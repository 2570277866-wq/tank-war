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

    // 数值均为"每秒"单位（px/s 或 rad/s）
    constexpr Attrs HEAVY = {
        .maxHP        = 200,
        .speed        = 100.0f,
        .rotateSpeed  = 3.0f,
        .damage       = 30,
        .bulletSpeed  = 350.0f,
        .skillCooldown = 15.0f,
        .skillDuration = 3.0f,
    };

    constexpr Attrs LIGHT = {
        .maxHP        = 150,
        .speed        = 160.0f,
        .rotateSpeed  = 5.0f,
        .damage       = 20,
        .bulletSpeed  = 480.0f,
        .skillCooldown = 12.0f,
        .skillDuration = 2.0f,
    };

    constexpr Attrs SCOUT = {
        .maxHP        = 100,
        .speed        = 240.0f,
        .rotateSpeed  = 7.0f,
        .damage       = 15,
        .bulletSpeed  = 600.0f,
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
    STEEL = 1,  // 钢墙：不可摧毁，不可通过
    GRASS = 2,  // 草丛：无碰撞，仅客户端视觉效果，坦克可穿行
    WATER = 3,  // 水域：不可通过，子弹可飞过
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

    // 默认地图障碍物布局（四类障碍物混合，对称竞技场）
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

        // ===== 边界钢墙 =====
        add(600, 5,   600, 5,   ObstacleType::STEEL);  // 上
        add(600, 795, 600, 5,   ObstacleType::STEEL);  // 下
        add(5,   400, 5,   400, ObstacleType::STEEL);  // 左
        add(1195, 400, 5,  400, ObstacleType::STEEL);  // 右

        // ===== 中央对称结构 =====
        // 中部钢墙掩体（左右对称）
        add(340, 200, 80, 12, ObstacleType::STEEL);
        add(860, 200, 80, 12, ObstacleType::STEEL);
        add(340, 600, 80, 12, ObstacleType::STEEL);
        add(860, 600, 80, 12, ObstacleType::STEEL);

        // 中央横钢条
        add(600, 350, 140, 10, ObstacleType::STEEL);
        add(600, 450, 140, 10, ObstacleType::STEEL);

        // ===== 水域（对称分布，阻隔通道但不挡子弹）=====
        add(180, 280, 50, 35, ObstacleType::WATER);
        add(1020, 280, 50, 35, ObstacleType::WATER);
        add(180, 520, 50, 35, ObstacleType::WATER);
        add(1020, 520, 50, 35, ObstacleType::WATER);

        add(600, 140, 90, 20, ObstacleType::WATER);
        add(600, 660, 90, 20, ObstacleType::WATER);

        // ===== 砖墙（可摧毁掩体）=====
        add(280, 160, 40, 40, ObstacleType::BRICK);
        add(920, 160, 40, 40, ObstacleType::BRICK);
        add(280, 640, 40, 40, ObstacleType::BRICK);
        add(920, 640, 40, 40, ObstacleType::BRICK);

        // 中场砖墙
        add(480, 280, 50, 12, ObstacleType::BRICK);
        add(720, 280, 50, 12, ObstacleType::BRICK);
        add(480, 520, 50, 12, ObstacleType::BRICK);
        add(720, 520, 50, 12, ObstacleType::BRICK);

        // 竖砖墙
        add(150, 400, 10, 80, ObstacleType::BRICK);
        add(1050, 400, 10, 80, ObstacleType::BRICK);

        // ===== 草丛（视觉掩护，无碰撞）=====
        add(440, 400, 55, 30, ObstacleType::GRASS);
        add(760, 400, 55, 30, ObstacleType::GRASS);

        add(250, 200, 35, 25, ObstacleType::GRASS);
        add(950, 200, 35, 25, ObstacleType::GRASS);
        add(250, 600, 35, 25, ObstacleType::GRASS);
        add(950, 600, 35, 25, ObstacleType::GRASS);

        add(600, 250, 30, 30, ObstacleType::GRASS);
        add(600, 550, 30, 30, ObstacleType::GRASS);
    }
}

namespace NetConfig {
    constexpr int RECV_BUF_SIZE = 4096;
    constexpr int SEND_BUF_SIZE = 4096;
}
