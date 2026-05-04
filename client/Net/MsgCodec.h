#pragma once

#include "../../Common/Protocol.h"
#include <cstring>
#include <string>

namespace MsgCodec {

struct LoginBody {
    char username[32];
    char password[32];
};

struct LoginAckBody {
    bool success;
    ErrorCode error;
};

inline void encodeLogin(const std::string& user, const std::string& pwd, char* out, uint16_t& outLen) {
    LoginBody body{};
    strncpy_s(body.username, user.c_str(), 31);
    strncpy_s(body.password, pwd.c_str(), 31);
    outLen = sizeof(LoginBody);
    memcpy(out, &body, outLen);
}

inline LoginAckBody decodeLoginAck(const char* data, uint16_t len) {
    LoginAckBody body{};
    if (len >= sizeof(LoginAckBody)) {
        memcpy(&body, data, sizeof(LoginAckBody));
    }
    return body;
}

inline void encodeInput(const InputState& input, char* out, uint16_t& outLen) {
    outLen = sizeof(InputState);
    memcpy(out, &input, outLen);
}

inline InputState decodeInput(const char* data, uint16_t len) {
    InputState input{};
    if (len >= sizeof(InputState)) {
        memcpy(&input, data, sizeof(InputState));
    }
    return input;
}

inline Snapshot decodeSnapshot(const char* data, uint16_t len) {
    Snapshot snap{};
    if (len >= sizeof(Snapshot)) {
        memcpy(&snap, data, sizeof(Snapshot));
    }
    return snap;
}

struct ShootBody {
    int    playerID;
    Vec2   pos;
    float  angle;
};

inline void encodeShoot(int playerID, Vec2 pos, float angle, char* out, uint16_t& outLen) {
    ShootBody body{ playerID, pos, angle };
    outLen = sizeof(ShootBody);
    memcpy(out, &body, outLen);
}

struct SkillBody {
    int       playerID;
    SkillType type;
};

inline void encodeSkill(int playerID, SkillType type, char* out, uint16_t& outLen) {
    SkillBody body{ playerID, type };
    outLen = sizeof(SkillBody);
    memcpy(out, &body, outLen);
}

struct JoinRoomBody {
    char username[32];
    TankType type;
};

inline void encodeJoinRoom(const std::string& user, TankType type, char* out, uint16_t& outLen) {
    JoinRoomBody body{};
    strncpy_s(body.username, user.c_str(), 31);
    body.type = type;
    outLen = sizeof(JoinRoomBody);
    memcpy(out, &body, outLen);
}

struct RankItem {
    char username[32];
    int  wins;
    int  losses;
    int  kills;
};

struct RankListBody {
    int      count;
    RankItem items[20];
};

inline RankListBody decodeRankList(const char* data, uint16_t len) {
    RankListBody body{};
    if (len >= sizeof(RankListBody)) {
        memcpy(&body, data, sizeof(RankListBody));
    }
    return body;
}

}
