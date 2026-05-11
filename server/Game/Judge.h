#pragma once

#include "../../Common/Protocol.h"
#include "../../Common/Config.h"

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

} // namespace Judge
