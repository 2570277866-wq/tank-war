// Collision.h
#pragma once

#include "Entity/Tank.h"
#include "Entity/Bullet.h"
#include <vector>

struct Circle {
    Vec2 center;
    float radius;
};

class Collision {
public:
    // ===== 圆形碰撞 ======
    // 两个圆是否相交
    static bool circleCircle(const Circle& a, const Circle& b);

    // ===== 子弹 vs 坦克 ======
    // 遍历所有子弹，检查是否命中坦克
    static bool bulletHitsTank(const Bullet& bullet, const Tank& tank);

    // ===== 坦克 vs 坦克 ======
    static bool tankTank(const Tank& a, const Tank& b, float radiusA, float radiusB);
};
