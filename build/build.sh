#!/bin/bash
# 坦克大战 - 快速编译脚本
# 绕过 cmake+make 中文路径乱码问题，直接用 g++ 编译
# 使用方法：在 UCRT64 终端中 cd 到项目根目录，执行 bash build/build.sh

set -e
cd "$(dirname "$0")/.."
mkdir -p build

echo "===== 编译客户端 (tank-war.exe) ====="
g++ -std=c++20 -O2 -I Common -I client -I "C:/msys64/ucrt64/include" -fexec-charset=UTF-8 \
  client/main.cpp \
  client/Core/GameLoop.cpp \
  client/Core/InputHandler.cpp \
  client/Core/StateInterpolator.cpp \
  client/Entity/Tank.cpp \
  client/Entity/Bullet.cpp \
  client/Effect/Collision.cpp \
  client/Effect/ParticleSystem.cpp \
  client/Effect/GameMap.cpp \
  client/Effect/DeathAnimator.cpp \
  client/Net/NetClient.cpp \
  client/UI/MenuUI.cpp \
  client/UI/LoginUI.cpp \
  client/UI/WaitUI.cpp \
  client/UI/Minimap.cpp \
  client/UI/GameOverUI.cpp \
  client/iob_shim.c \
  -o build/tank-war.exe \
  -L"C:/msys64/ucrt64/lib" -leasyx -luser32 -lgdi32 -lwinmm -lws2_32 \
  -Wl,--defsym=__imp___iob_func=__iob_func

echo "===== 编译服务端 (tank_server.exe) ====="
g++ -std=c++20 -O2 -I Common -I server -fexec-charset=UTF-8 \
  server/main.cpp \
  server/Net/TCPServer.cpp \
  server/Net/Session.cpp \
  server/Game/Room.cpp \
  server/Game/GameWorld.cpp \
  server/Data/Leaderboard.cpp \
  server/Data/RecordManager.cpp \
  server/Data/UserManager.cpp \
  -o build/tank_server.exe \
  -luser32 -lgdi32 -lwinmm -lws2_32 -liphlpapi

echo ""
echo "===== 编译完成 ====="
ls -lh build/tank-war.exe build/tank_server.exe
