#pragma once

#include "Protocol.h"
#include "Config.h"
#include <graphics.h>

class Minimap {
public:
    static void draw(Vec2 playerPos, Vec2 enemyPos,
                     bool playerAlive, bool enemyAlive);
};
