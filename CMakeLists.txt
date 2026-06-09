cmake_minimum_required(VERSION 3.15)
project(tank-war LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/client)
include_directories(${CMAKE_SOURCE_DIR}/Common)

# EasyX (MSYS2 UCRT64) include/lib paths
if(MINGW)
    include_directories("C:/msys64/ucrt64/include")
    link_directories("C:/msys64/ucrt64/lib")
endif()

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

if(MSVC)
    target_compile_definitions(tank-war PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_compile_options(tank-war PRIVATE /utf-8)
endif()

target_link_libraries(tank-war PRIVATE
    user32
    gdi32
    winmm
    ws2_32
    easyx
)

# MinGW: EasyX is ANSI-only; source is UTF-8, need GBK for GDI on Chinese Windows
# This flag ONLY applies to the client (GUI with EasyX), NOT the server
if(MINGW)
    target_compile_options(tank-war PRIVATE -fexec-charset=GBK)
endif()

# MinGW: 旧版 EasyX 引用 __imp___iob_func，新版 UCRT 已移除该符号
# iob_shim.c 提供 __iob_func 本地实现，--defsym 将 import-stub 别名过来
if(MINGW)
    target_sources(tank-war PRIVATE client/iob_shim.c)
    target_link_options(tank-war PRIVATE -Wl,--defsym=__imp___iob_func=__iob_func)
endif()

# ====== 服务端 ======
include_directories(${CMAKE_SOURCE_DIR}/server)

file(GLOB SERVER_SOURCES
    server/main.cpp
    server/Net/TCPServer.cpp
    server/Net/Session.cpp
    server/Game/Room.cpp
    server/Game/GameWorld.cpp
    server/Data/Leaderboard.cpp
    server/Data/RecordManager.cpp
    server/Data/UserManager.cpp
)

add_executable(tank_server ${SERVER_SOURCES})

if(MSVC)
    target_compile_definitions(tank_server PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_compile_options(tank_server PRIVATE /utf-8)
endif()

# MinGW: 服务端源码是 UTF-8，控制台也设为 UTF-8，显式确保执行字符集为 UTF-8
if(MINGW)
    target_compile_options(tank_server PRIVATE -fexec-charset=UTF-8)
endif()

target_link_libraries(tank_server PRIVATE
    user32
    gdi32
    winmm
    ws2_32
    iphlpapi
)
