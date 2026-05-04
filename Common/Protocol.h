#pragma once

#include <cstdint>

enum class MsgID : uint16_t {
    C2S_LOGIN        = 0x0100,
    C2S_REGISTER     = 0x0101,
    S2C_LOGIN_ACK    = 0x0102,
    S2C_REGISTER_ACK = 0x0103,

    C2S_JOIN_ROOM    = 0x0200,
    C2S_LEAVE_ROOM   = 0x0201,
    S2C_ROOM_INFO    = 0x0202,
    S2C_MATCH_RESULT = 0x0203,

    C2S_INPUT        = 0x0300,
    S2C_SNAPSHOT     = 0x0301,
    C2S_SHOOT        = 0x0302,
    C2S_USE_SKILL    = 0x0303,
    S2C_HIT          = 0x0304,
    S2C_SKILL_EFFECT = 0x0305,

    S2C_GAME_OVER    = 0x0400,
    C2S_GET_RANK     = 0x0401,
    S2C_RANK_LIST    = 0x0402,

    C2S_HEARTBEAT    = 0x0500,
    S2C_HEARTBEAT    = 0x0501,
    S2C_ERROR        = 0x0FFF,
};

struct MsgHeader {
    MsgID    id;
    uint16_t bodyLen;
};

struct Vec2 {
    float x;
    float y;
};

enum class TankType : uint8_t {
    HEAVY = 0,
    LIGHT = 1,
    SCOUT = 2,
};

enum class RoomState : uint8_t {
    WAITING = 0,
    READY   = 1,
    PLAYING = 2,
};

enum class SkillType : uint8_t {
    SHIELD  = 0,
    SPRINT  = 1,
    SCATTER = 2,
};

struct TankState {
    Vec2     pos;
    float    angle;
    int      curHP;
    int      maxHP;
    float    skillCooldown;
    float    skillTimer;
    TankType type;
    int      playerID;
    bool     alive;
    bool     shieldActive;
    bool     sprintActive;
};

struct BulletState {
    Vec2 pos;
    Vec2 vel;
    int  owner;
    int  damage;
};

struct SkillEffectState {
    int       playerID;
    SkillType type;
    float     timer;
};

struct Snapshot {
    uint32_t          frameSeq;
    TankState         tanks[2];
    BulletState       bullets[64];
    int               bulletCount;
    SkillEffectState  skills[2];
};

struct InputState {
    bool w, a, s, d;
    bool space;
    bool f;
};

enum class ErrorCode : uint16_t {
    NONE             = 0x0000,
    LOGIN_FAILED     = 0x0001,
    REGISTER_EXISTS  = 0x0002,
    ROOM_FULL        = 0x0003,
    INVALID_MSG      = 0x0004,
    KICKED           = 0x0005,
};

constexpr uint16_t SERVER_PORT    = 9527;
constexpr int      TICK_RATE      = 20;
constexpr int      TICK_INTERVAL  = 1000 / TICK_RATE;
constexpr int      RENDER_FPS    = 60;
constexpr int      MAX_BULLETS   = 64;
constexpr int      HEARTBEAT_MS  = 3000;
constexpr int      RECONNECT_MS  = 10000;


