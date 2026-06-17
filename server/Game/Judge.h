#pragma once

#include "../../Common/Protocol.h"
#include "../../Common/Config.h"
#include <algorithm>
#include <cmath>

namespace Judge {

inline bool CircleCircle(Vec2 aPos, float aR, Vec2 bPos, float bR) {
    float dx = aPos.x - bPos.x;
    float dy = aPos.y - bPos.y;
    float distSq = dx * dx + dy * dy;
    float radii = aR + bR;
    return distSq <= radii * radii;
}

inline bool InBounds(Vec2 pos, float radius) {
    return pos.x - radius >= 0 &&
           pos.x + radius <= static_cast<float>(MapConfig::WIDTH) &&
           pos.y - radius >= 0 &&
           pos.y + radius <= static_cast<float>(MapConfig::HEIGHT);
}

inline void ClampToBounds(Vec2& pos, float radius) {
    if (pos.x - radius < 0) pos.x = radius;
    if (pos.x + radius > static_cast<float>(MapConfig::WIDTH))
        pos.x = static_cast<float>(MapConfig::WIDTH) - radius;
    if (pos.y - radius < 0) pos.y = radius;
    if (pos.y + radius > static_cast<float>(MapConfig::HEIGHT))
        pos.y = static_cast<float>(MapConfig::HEIGHT) - radius;
}

// 圆与AABB矩形碰撞检测
inline bool CircleRect(Vec2 circlePos, float radius,
                       Vec2 rectPos, Vec2 rectHalfSize) {
    float halfW = rectHalfSize.x;
    float halfH = rectHalfSize.y;

    float closestX = (std::max)(rectPos.x - halfW,
                          (std::min)(circlePos.x, rectPos.x + halfW));
    float closestY = (std::max)(rectPos.y - halfH,
                          (std::min)(circlePos.y, rectPos.y + halfH));

    float dx = circlePos.x - closestX;
    float dy = circlePos.y - closestY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

// 将圆推离AABB矩形（用于坦克碰撞响应）
inline void PushCircleOutOfRect(Vec2& circlePos, float radius,
                                 Vec2 rectPos, Vec2 rectHalfSize) {
    float halfW = rectHalfSize.x;
    float halfH = rectHalfSize.y;

    float closestX = (std::max)(rectPos.x - halfW,
                          (std::min)(circlePos.x, rectPos.x + halfW));
    float closestY = (std::max)(rectPos.y - halfH,
                          (std::min)(circlePos.y, rectPos.y + halfH));

    float dx = circlePos.x - closestX;
    float dy = circlePos.y - closestY;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < radius && dist > 0.0001f) {
        float overlap = radius - dist;
        circlePos.x += (dx / dist) * overlap;
        circlePos.y += (dy / dist) * overlap;
    }
}

}
