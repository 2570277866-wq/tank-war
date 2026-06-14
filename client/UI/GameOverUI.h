#pragma once

#include <graphics.h>
#include <string>

enum class GameOverResult {
    PLAY_AGAIN,
    MAIN_MENU
};

struct GameOverInfo {
    bool   playerWin;
    int    playerKills;
    int    playerDamage;
    float  duration;
};

class GameOverUI {
public:
    static GameOverResult show(const GameOverInfo& info);
};
