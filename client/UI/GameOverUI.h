#pragma once

#include <graphics.h>
#include <string>

struct GameOverInfo {
    bool   playerWin;
    int    playerKills;
    int    playerDamage;
    float  duration;
};

class GameOverUI {
public:
    static void show(const GameOverInfo& info);
};
