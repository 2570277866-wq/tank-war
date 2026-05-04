#include "Minimap.h"

void Minimap::draw(Vec2 playerPos, Vec2 enemyPos,
                    bool playerAlive, bool enemyAlive) {
    int mx = MapConfig::WIDTH - 110;
    int my = MapConfig::HEIGHT - 90;
    int mw = 100;
    int mh = 75;

    setfillcolor(RGB(20, 30, 20));
    setlinecolor(RGB(80, 80, 80));
    setlinestyle(PS_SOLID, 1);
    fillrectangle(mx, my, mx + mw, my + mh);

    if (playerAlive) {
        int px = mx + (int)(playerPos.x / MapConfig::WIDTH * mw);
        int py = my + (int)(playerPos.y / MapConfig::HEIGHT * mh);
        setfillcolor(RGB(50, 150, 255));
        fillcircle(px, py, 3);
    }

    if (enemyAlive) {
        int ex = mx + (int)(enemyPos.x / MapConfig::WIDTH * mw);
        int ey = my + (int)(enemyPos.y / MapConfig::HEIGHT * mh);
        setfillcolor(RGB(255, 50, 50));
        fillcircle(ex, ey, 3);
    }
}
