#pragma once

#include "Protocol.h"
#include "Config.h"
#include <graphics.h>
#include <vector>

struct ServerObstacle {
    Vec2         pos;
    Vec2         size;   // 半宽半高（与服务器一致）
    ObstacleType type;
    int          curHP;
    int          maxHP;
    bool         destroyed;
};

class GameMap {
public:
    GameMap();

    void draw();
    void drawGrassOverlay();

    bool checkBulletCollision(Vec2 pos, float radius, int damage);

    // 坦克碰撞：与服务器完全相同的算法
    bool checkTankCollision(Vec2& pos, float radius);
    void clampToBounds(Vec2& pos, float radius);

    const std::vector<ServerObstacle>& getObstacles() const { return m_obstacles; }

    void applyObstacleDestroyed(int count, const bool* destroyed);

private:
    std::vector<ServerObstacle> m_obstacles;
};
