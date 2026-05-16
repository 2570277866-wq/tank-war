cmake_minimum_required(VERSION 3.15)
project(tank-war LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/client)
include_directories(${CMAKE_SOURCE_DIR}/Common)

file(GLOB CLIENT_SOURCES
    client/main.cpp
    client/Core/GameLoop.cpp
    client/Core/InputHandler.cpp
    client/Core/StateInterpolator.cpp
    client/Entity/Tank.cpp
    client/Entity/Bullet.cpp
    client/Effect/Collision.cpp
    client/Effect/ParticleSystem.cpp
    client/Effect/GameMap.cpp
    client/Effect/DeathAnimator.cpp
    client/Net/NetClient.cpp
    client/UI/MenuUI.cpp
    client/UI/LoginUI.cpp
    client/UI/WaitUI.cpp
    client/UI/Minimap.cpp
    client/UI/GameOverUI.cpp
)

add_executable(tank-war ${CLIENT_SOURCES})

target_compile_definitions(tank-war PRIVATE _CRT_SECURE_NO_WARNINGS)

target_link_libraries(tank-war PRIVATE
    user32
    gdi32
    winmm
    ws2_32
    easyx
)
