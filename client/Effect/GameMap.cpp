#include "GameMap.h"
#include "../../Common/Config.h"
#include <cmath>

GameMap::GameMap() {
    int T = 40;

    for (int i = 0; i < 4; i++) {
        m_walls.push_back({ 200, 120 + i * T, T, T, WallType::BRICK, 2 });
    }
    for (int i = 0; i < 4; i++) {
        m_walls.push_back({ 560, 120 + i * T, T, T, WallType::BRICK, 2 });
    }

    m_walls.push_back({ 340, 160, T * 3, T, WallType::STEEL, 999 });
    m_walls.push_back({ 340, 400, T * 3, T, WallType::STEEL, 999 });

    for (int i = 0; i < 3; i++) {
        m_walls.push_back({ 100 + i * T, 350, T, T, WallType::GRASS, 999 });
    }
    for (int i = 0; i < 3; i++) {
        m_walls.push_back({ 600 + i * T, 350, T, T, WallType::GRASS, 999 });
    }

    m_walls.push_back({ 360, 260, T * 2, T, WallType::WATER, 999 });
}

void GameMap::draw() {
    for (auto& w : m_walls) {
        if (w.type == WallType::GRASS) continue;

        switch (w.type) {
            case WallType::BRICK:
                if (w.hp > 0) {
                    setfillcolor(RGB(160, 82, 45));
                    fillrectangle(w.x, w.y, w.x + w.w, w.y + w.h);
                    setlinecolor(RGB(120, 60, 30));
                    setlinestyle(PS_SOLID, 1);
                    for (int i = 1; i < w.w / 10; i++) {
                        line(w.x + i * 10, w.y, w.x + i * 10, w.y + w.h);
                    }
                    for (int i = 1; i < w.h / 10; i++) {
                        line(w.x, w.y + i * 10, w.x + w.w, w.y + i * 10);
                    }
                }
                break;
            case WallType::STEEL:
                setfillcolor(RGB(130, 130, 140));
                fillrectangle(w.x, w.y, w.x + w.w, w.y + w.h);
                setlinecolor(RGB(180, 180, 190));
                setlinestyle(PS_SOLID, 2);
                rectangle(w.x + 2, w.y + 2, w.x + w.w - 2, w.y + w.h - 2);
                break;
            case WallType::WATER:
                setfillcolor(RGB(30, 80, 160));
                fillrectangle(w.x, w.y, w.x + w.w, w.y + w.h);
                setlinecolor(RGB(50, 100, 200));
                setlinestyle(PS_SOLID, 1);
                for (int i = 0; i < w.w; i += 8) {
                    line(w.x + i, w.y + w.h / 2, w.x + i + 4, w.y + w.h / 2);
                }
                break;
            default:
                break;
        }
    }
}

void GameMap::drawGrassOverlay() {
    for (auto& w : m_walls) {
        if (w.type == WallType::GRASS) {
            setfillcolor(RGB(34, 120, 34));
            fillrectangle(w.x, w.y, w.x + w.w, w.y + w.h);
            setlinecolor(RGB(50, 150, 50));
            setlinestyle(PS_SOLID, 1);
            for (int i = 0; i < w.w; i += 6) {
                line(w.x + i, w.y + w.h, w.x + i + 2, w.y + w.h - 8);
            }
        }
    }
}

bool GameMap::rectCircleCollision(int rx, int ry, int rw, int rh,
                                  Vec2 center, float radius) {
    float closestX = std::max((float)rx, std::min(center.x, (float)(rx + rw)));
    float closestY = std::max((float)ry, std::min(center.y, (float)(ry + rh)));
    float dx = center.x - closestX;
    float dy = center.y - closestY;
    return (dx * dx + dy * dy) < (radius * radius);
}

bool GameMap::checkTankCollision(Vec2 pos, float radius, Vec2& outCorrected) {
    outCorrected = pos;
    for (auto& w : m_walls) {
        if (w.hp <= 0) continue;
        if (w.type == WallType::GRASS) continue;

        if (rectCircleCollision(w.x, w.y, w.w, w.h, pos, radius)) {
            float cx = w.x + w.w / 2.0f;
            float cy = w.y + w.h / 2.0f;
            float dx = pos.x - cx;
            float dy = pos.y - cy;

            if (std::abs(dx) / (w.w / 2.0f + radius) > std::abs(dy) / (w.h / 2.0f + radius)) {
                outCorrected.x = (dx > 0) ? (w.x + w.w + radius) : (w.x - radius);
                outCorrected.y = pos.y;
            } else {
                outCorrected.x = pos.x;
                outCorrected.y = (dy > 0) ? (w.y + w.h + radius) : (w.y - radius);
            }
            return true;
        }
    }
    return false;
}

bool GameMap::checkBulletCollision(Vec2 pos, float radius, int damage) {
    for (auto& w : m_walls) {
        if (w.hp <= 0) continue;
        if (w.type == WallType::GRASS) continue;

        if (rectCircleCollision(w.x, w.y, w.w, w.h, pos, radius)) {
            if (w.type == WallType::BRICK) {
                w.hp -= damage;
                if (w.hp <= 0) w.hp = 0;
            }
            return true;
        }
    }
    return false;
}
