#pragma once

#include <string>
#include <functional>
#include <atomic>
#include "../../Common/Protocol.h"
#include "../Core/Clock.h"
#include "GameWorld.h"

class Room {
public:
    RoomState   state = RoomState::WAITING;

    int         playerIDs[2]       = {-1, -1};
    std::string playerNames[2]     = {"", ""};
    TankType    selectedTanks[2];
    bool        tankSelected[2]    = {false, false};

    GameWorld   world;

    int64_t     disconnectTimeUs[2] = {0, 0};
    int64_t     lastHeartbeatUs[2]  = {0, 0};
    int64_t     gameStartUs = 0;
    std::atomic<bool> disconnectPending[2] = {false, false};

    std::function<void(int playerID, MsgID, const void*, uint16_t)> sendToPlayer;

    bool Join(int playerID, const std::string& username);
    void Leave(int playerID);
    bool IsFull() const;
    bool IsPlaying() const;
    int  GetSlot(int playerID) const;

    bool SelectTank(int playerID, TankType type);
    void StartGame();

    void Tick(float dt);

    void OnPlayerInput(int playerID, const InputState& input);
    void OnPlayerShoot(int playerID);
    void OnPlayerSkill(int playerID);
    void OnHeartbeat(int playerID);

    void HandleDisconnect(int playerID);
    bool TryReconnect(const std::string& username, int newPlayerID);
    void ForfeitPlayer(int playerID);

private:
    void BroadcastSnapshot();
    void CheckDisconnectTimeout(float dt);
    void EndGame(int winnerSlot, bool forfeit);
    void SendToSlot(int slot, MsgID id, const void* body, uint16_t len);
};
