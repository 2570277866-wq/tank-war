#pragma once

#include "../../Common/Protocol.h"

// 房间：2个位置，状态机 WAITING → READY → PLAYING
class Room
{
public:
    // 房间状态（与协议中 RoomState 枚举对应）
    RoomState state = RoomState::WAITING;

    // 两个玩家槽位，-1 表示空位
    int playerIDs[2] = {-1, -1};

    // 玩家加入房间，满员自动切 READY
    bool Join(int playerID);

    // 玩家离开房间，切回 WAITING
    void Leave(int playerID);

    bool IsFull() const;
};
