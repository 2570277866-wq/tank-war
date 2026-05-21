#pragma once

#include "Protocol.h"
#include "Config.h"
#include <graphics.h>
#include <vector>

enum class WallType : uint8_t {
    BRICK  = 0,
    STEEL  = 1,
    GRASS  = 2,
    WATER  = 3,
};

struct Wall {
    int      x, y;
    int      w, h;
    WallType type;
    int      hp;
};

class GameMap {
public:
    GameMap();

    void draw();
    void drawGrassOverlay();

    bool checkTankCollision(Vec2 pos, float radius, Vec2& outCorrected);
    bool checkBulletCollision(Vec2 pos, float radius, int damage);

    const std::vector<Wall>& getWalls() const { return m_walls; }

private:
    std::vector<Wall> m_walls;

    bool rectCircleCollision(int rx, int ry, int rw, int rh,
                             Vec2 center, float radius);
};
