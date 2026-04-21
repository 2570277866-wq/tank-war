// main.cpp
#pragma once

#include "Core/GameLoop.h"
#include "Entity/Tank.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "Core/GameLoop.h"





int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  双人联机坦克大战 - 逻辑演示程序" << std::endl;
    std::cout << "========================================" << std::endl;

    // ===== 选择坦克类型 =====
    std::cout << "\n选择你的坦克类型：" << std::endl;
    std::cout << "  0 - HeavyTank (高血量 低速 大伤害)" << std::endl;
    std::cout << "  1 - LightTank  (平衡型)" << std::endl;
    std::cout << "  2 - ScoutTank  (高速 低血量 强技能)" << std::endl;
    std::cout << "请输入 (0/1/2): ";

    int choice;
    std::cin >> choice;

    TankType type;
    switch (choice) {
        case 0: type = TankType::HEAVY; std::cout << "选择了 HeavyTank！\n"; break;
        case 1: type = TankType::LIGHT; std::cout << "选择了 LightTank！\n"; break;
        case 2: type = TankType::SCOUT; std::cout << "选择了 ScoutTank！\n"; break;
        default: type = TankType::LIGHT; std::cout << "默认 LightTank。\n"; break;
    }

    // ===== 初始化游戏 ======
    GameLoop game;
    game.setLocalTankType(type);
    game.start();

    // ===== 模拟游戏循环（60fps）=====
    std::cout << "\n游戏运行中... (纯逻辑模式，无图形)" << std::endl;
    std::cout << "每1秒打印一次状态...\n" << std::endl;

    int tickCount = 0;
    while (game.isRunning() && !game.isGameOver()) {
        // 模拟 60fps：每帧约 16ms
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        game.update(0.016f); // dt = 16ms = 0.016s
        tickCount++;

        // 每秒打印一次状态
        if (tickCount % 60 == 0) {
            Tank* p = game.getLocalTank();
            Tank* e = game.getEnemyTank();
            std::cout << "--- Tick " << tickCount/60 << "s ---" << std::endl;
            std::cout << "  玩家: HP=" << p->getCurHP() << "/" << p->getMaxHP()
                      << " 位置=(" << (int)p->getPos().x << "," << (int)p->getPos().y << ")"
                      << " 存活=" << (p->isAlive()?"是":"否") << std::endl;
            std::cout << "  敌人: HP=" << e->getCurHP() << "/" << e->getMaxHP()
                      << " 位置=(" << (int)e->getPos().x << "," << (int)e->getPos().y << ")"
                      << " 存活=" << (e->isAlive()?"是":"否") << std::endl;
            std::cout << "  子弹数: " << game.getBullets().size() << std::endl;
        }
    }

    std::cout << "\n游戏结束！按回车退出..." << std::endl;
    std::getchar();
    return 0;
}
