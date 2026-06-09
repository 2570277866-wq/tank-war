#include "GameMap.h"
#include <cmath>

// ===== 与服务器 Judge 完全一致的碰撞函数 =====
static bool circleRect(Vec2 circlePos, float radius, Vec2 rectPos, Vec2 rectHalf) {
    float closestX = std::max(rectPos.x - rectHalf.x, std::min(circlePos.x, rectPos.x + rectHalf.x));
    float closestY = std::max(rectPos.y - rectHalf.y, std::min(circlePos.y, rectPos.y + rectHalf.y));
    float dx = circlePos.x - closestX;
    float dy = circlePos.y - closestY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

static void pushCircleOutOfRect(Vec2& pos, float radius, Vec2 rectPos, Vec2 rectHalf) {
    float closestX = std::max(rectPos.x - rectHalf.x, std::min(pos.x, rectPos.x + rectHalf.x));
    float closestY = std::max(rectPos.y - rectHalf.y, std::min(pos.y, rectPos.y + rectHalf.y));
    float dx = pos.x - closestX;
    float dy = pos.y - closestY;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < radius && dist > 0.0001f) {
        float overlap = radius - dist;
        pos.x += (dx / dist) * overlap;
        pos.y += (dy / dist) * overlap;
    }
}

GameMap::GameMap() {
    Obstacle obstacles[MapConfig::MAX_OBSTACLES];
    int count = 0;
    MapConfig::GetDefaultObstacles(obstacles, count);

    for (int i = 0; i < count; i++) {
        ServerObstacle w;
        w.pos      = obstacles[i].pos;
        w.size     = obstacles[i].size;
        w.type     = obstacles[i].type;
        w.curHP    = obstacles[i].curHP;
        w.maxHP    = obstacles[i].maxHP;
        w.destroyed = obstacles[i].destroyed;
        m_obstacles.push_back(w);
    }
}

void GameMap::applyObstacleDestroyed(int count, const bool* destroyed) {
    for (int i = 0; i < count && i < (int)m_obstacles.size(); i++) {
        if (destroyed[i] && m_obstacles[i].type == ObstacleType::BRICK) {
            m_obstacles[i].destroyed = true;
            m_obstacles[i].curHP = 0;
        }
    }
}

bool GameMap::checkTankCollision(Vec2& pos, float radius) {
    bool collided = false;
    for (auto& w : m_obstacles) {
        if (w.destroyed) continue;
        if (w.type == ObstacleType::GRASS) continue;

        if (circleRect(pos, radius, w.pos, w.size)) {
            pushCircleOutOfRect(pos, radius, w.pos, w.size);
            collided = true;
        }
    }
    return collided;
}

void GameMap::clampToBounds(Vec2& pos, float radius) {
    if (pos.x - radius < 0) pos.x = radius;
    if (pos.x + radius > (float)MapConfig::WIDTH) pos.x = (float)MapConfig::WIDTH - radius;
    if (pos.y - radius < 0) pos.y = radius;
    if (pos.y + radius > (float)MapConfig::HEIGHT) pos.y = (float)MapConfig::HEIGHT - radius;
}

void GameMap::draw() {
    for (auto& w : m_obstacles) {
        if (w.type == ObstacleType::GRASS) continue;
        if (w.destroyed) continue;

        int left   = (int)(w.pos.x - w.size.x);
        int top    = (int)(w.pos.y - w.size.y);
        int right  = (int)(w.pos.x + w.size.x);
        int bottom = (int)(w.pos.y + w.size.y);

        switch (w.type) {
            case ObstacleType::BRICK:
                setfillcolor(RGB(160, 82, 45));
                fillrectangle(left, top, right, bottom);
                setlinecolor(RGB(120, 60, 30));
                setlinestyle(PS_SOLID, 1);
                for (int i = 1; i < (int)(w.size.x * 2) / 10; i++) {
                    line(left + i * 10, top, left + i * 10, bottom);
                }
                for (int i = 1; i < (int)(w.size.y * 2) / 10; i++) {
                    line(left, top + i * 10, right, top + i * 10);
                }
                break;
            case ObstacleType::STEEL:
                setfillcolor(RGB(130, 130, 140));
                fillrectangle(left, top, right, bottom);
                setlinecolor(RGB(180, 180, 190));
                setlinestyle(PS_SOLID, 2);
                rectangle(left + 2, top + 2, right - 2, bottom - 2);
                break;
            case ObstacleType::WATER:
                setfillcolor(RGB(30, 80, 160));
                fillrectangle(left, top, right, bottom);
                setlinecolor(RGB(50, 110, 220));
                setlinestyle(PS_SOLID, 1);
                for (int wy = top + 5; wy < bottom; wy += 10) {
                    for (int wx = left + (wy % 20 == 0 ? 0 : 5); wx < right; wx += 15) {
                        line(wx, wy, wx + 6, wy);
                    }
                }
                break;
            default:
                break;
        }
    }
}

void GameMap::drawGrassOverlay() {
    for (auto& w : m_obstacles) {
        if (w.type == ObstacleType::GRASS) {
            int left   = (int)(w.pos.x - w.size.x);
            int top    = (int)(w.pos.y - w.size.y);
            int right  = (int)(w.pos.x + w.size.x);
            int bottom = (int)(w.pos.y + w.size.y);

            setfillcolor(RGB(34, 120, 34));
            fillrectangle(left, top, right, bottom);
            setlinecolor(RGB(50, 150, 50));
            setlinestyle(PS_SOLID, 1);
            int width = right - left;
            for (int i = 0; i < width; i += 6) {
                line(left + i, bottom, left + i + 2, bottom - 8);
            }
        }
    }
}

bool GameMap::checkBulletCollision(Vec2 pos, float radius, int damage) {
    for (auto& w : m_obstacles) {
        if (w.destroyed) continue;
        if (w.type == ObstacleType::GRASS) continue;

        if (circleRect(pos, radius, w.pos, w.size)) {
            if (w.type == ObstacleType::BRICK) {
                w.curHP -= damage;
                if (w.curHP <= 0) {
                    w.curHP = 0;
                    w.destroyed = true;
                }
            }
            return true;
        }
    }
    return false;
}
