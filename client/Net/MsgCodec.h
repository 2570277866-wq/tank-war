#pragma once

#include "../../Common/Protocol.h"
#include <cstring>
#include <string>

namespace MsgCodec {

struct LoginBody {
    char username[32];
    char password[32];
};

inline void encodeLogin(const std::string& user, const std::string& pwd, char* out, uint16_t& outLen) {
    LoginBody body{};
    strncpy_s(body.username, user.c_str(), 31);
    strncpy_s(body.password, pwd.c_str(), 31);
    outLen = sizeof(LoginBody);
    memcpy(out, &body, outLen);
}

inline ErrorCode decodeLoginAck(const char* data, uint16_t len) {
    ErrorCode code = ErrorCode::LOGIN_FAILED;
    if (len >= sizeof(ErrorCode)) {
        memcpy(&code, data, sizeof(ErrorCode));
    }
    return code;
}

inline void encodeInput(const InputState& input, char* out, uint16_t& outLen) {
    outLen = sizeof(InputState);
    memcpy(out, &input, outLen);
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

inline void encodeSelectTank(TankType type, char* out, uint16_t& outLen) {
    SelectTankReq req{ type };
    outLen = sizeof(SelectTankReq);
    memcpy(out, &req, outLen);
}

inline MatchResultData decodeMatchResult(const char* data, uint16_t len) {
    MatchResultData match{};
    if (len >= sizeof(MatchResultData)) {
        memcpy(&match, data, sizeof(MatchResultData));
    }
    return match;
}

inline HitData decodeHitData(const char* data, uint16_t len) {
    HitData hd{};
    if (len >= sizeof(HitData)) {
        memcpy(&hd, data, sizeof(HitData));
    }
    return hd;
}

inline GameOverData decodeGameOverData(const char* data, uint16_t len) {
    GameOverData gd{};
    if (len >= sizeof(GameOverData)) {
        memcpy(&gd, data, sizeof(GameOverData));
    }
    return gd;
}

inline void encodeReconnect(const std::string& username, char* out, uint16_t& outLen) {
    ReconnectReq req{};
    strncpy_s(req.username, username.c_str(), 31);
    outLen = sizeof(ReconnectReq);
    memcpy(out, &req, outLen);
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
