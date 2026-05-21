// Collision.cpp
#include "Collision.h"
#include "Config.h"
#include <cmath>

bool Collision::circleCircle(const Circle& a, const Circle& b) {
    float dx = a.center.x - b.center.x;
    float dy = a.center.y - b.center.y;
    float distSq = dx * dx + dy * dy;
    float radiusSum = a.radius + b.radius;
    return distSq <= radiusSum * radiusSum;
}

bool Collision::bulletHitsTank(const Bullet& bullet, const Tank& tank) {
    if (!bullet.isAlive() || !tank.isAlive()) return false;
    Circle bulletCircle = { bullet.getPos(), BULLET_RADIUS };
    Circle tankCircle   = { tank.getPos(), CollisionConfig::TANK_RADIUS }; // TANK_RADIUS
    return circleCircle(bulletCircle, tankCircle);
}

bool Collision::tankTank(const Tank& a, const Tank& b, float radiusA, float radiusB) {
    if (!a.isAlive() || !b.isAlive()) return false;
    Circle ca = { a.getPos(), radiusA };
    Circle cb = { b.getPos(), radiusB };
    return circleCircle(ca, cb);
}
