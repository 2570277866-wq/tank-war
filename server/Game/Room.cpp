#include "Room.h"
#include "../Data/RecordManager.h"
#include "../Data/UserManager.h"
#include "../Core/Logger.h"
#include <iostream>
#include <ctime>

using namespace std;

bool Room::Join(int playerID, const std::string& username) {
    if (IsFull()) return false;

    for (int i = 0; i < 2; ++i) {
        if (playerIDs[i] == -1) {
            playerIDs[i] = playerID;
            playerNames[i] = username;
            tankSelected[i] = false;
            disconnectPending[i] = false;
            lastHeartbeatUs[i] = Clock::Now();

            if (IsFull()) state = RoomState::READY;
            return true;
        }
    }
    return false;
}

void Room::Leave(int playerID) {
    for (int i = 0; i < 2; ++i) {
        if (playerIDs[i] == playerID) {
            // If game in progress, other player wins by forfeit
            if (state == RoomState::PLAYING || state == RoomState::PAUSED) {
                int other = 1 - i;
                if (playerIDs[other] != -1) {
                    EndGame(other, true);
                    return; // EndGame already resets room state
                }
            }
            playerIDs[i] = -1;
            playerNames[i] = "";
            tankSelected[i] = false;
            disconnectPending[i] = false;
            state = RoomState::WAITING;
            return;
        }
    }
}

bool Room::IsFull() const {
    return playerIDs[0] != -1 && playerIDs[1] != -1;
}

bool Room::IsPlaying() const {
    return state == RoomState::PLAYING || state == RoomState::PAUSED;
}

int Room::GetSlot(int playerID) const {
    if (playerIDs[0] == playerID) return 0;
    if (playerIDs[1] == playerID) return 1;
    return -1;
}

bool Room::SelectTank(int playerID, TankType type) {
    int slot = GetSlot(playerID);
    if (slot < 0) return false;
    if (tankSelected[slot]) return false; // already selected

    selectedTanks[slot] = type;
    tankSelected[slot] = true;

    // Ack to player
    ErrorCode code = ErrorCode::NONE;
    SendToSlot(slot, MsgID::S2C_SELECT_TANK_ACK, &code, sizeof(code));

    Logger::Get().Game("玩家 " + playerNames[slot] +
                       " 选择坦克类型=" + std::to_string((int)type));

    // Both players have selected, start game
    if (tankSelected[0] && tankSelected[1]) {
        StartGame();
    }
    return true;
}

void Room::StartGame() {
    world.Init(playerIDs[0], selectedTanks[0],
               playerIDs[1], selectedTanks[1]);
    state = RoomState::PLAYING;
    gameStartUs = Clock::Now();

    for (int i = 0; i < 2; ++i) {
        lastHeartbeatUs[i] = Clock::Now();
    }

    // Send match result to both players
    MatchResultData match;
    match.playerIDs[0] = playerIDs[0];
    match.playerIDs[1] = playerIDs[1];
    match.tankTypes[0] = selectedTanks[0];
    match.tankTypes[1] = selectedTanks[1];
    match.startPositions[0] = world.tanks[0].pos;
    match.startPositions[1] = world.tanks[1].pos;

    SendToSlot(0, MsgID::S2C_MATCH_RESULT, &match, sizeof(match));
    SendToSlot(1, MsgID::S2C_MATCH_RESULT, &match, sizeof(match));

    Logger::Get().Game("游戏开始！" + playerNames[0] + " vs " + playerNames[1]);
}

void Room::Tick(float dt) {
    if (state == RoomState::PLAYING) {
        world.Tick(dt);

        // Send hit events
        for (int i = 0; i < world.hitEventCount; ++i) {
            const HitEvent& ev = world.hitEvents[i];
            HitData hd;
            hd.victimID    = world.tanks[ev.victimSlot].playerID;
            hd.damage      = ev.damage;
            hd.remainingHP = ev.remainingHP;
            hd.attackerID  = world.tanks[ev.attackerSlot].playerID;
            SendToSlot(0, MsgID::S2C_HIT, &hd, sizeof(hd));
            SendToSlot(1, MsgID::S2C_HIT, &hd, sizeof(hd));
        }

        BroadcastSnapshot();

        if (world.gameOver) {
            int winnerSlot = (world.winnerID == playerIDs[0]) ? 0 : 1;
            EndGame(winnerSlot, world.winByForfeit);
        }
    }

    if (state == RoomState::PAUSED) {
        CheckDisconnectTimeout(dt);
    }
}

void Room::BroadcastSnapshot() {
    Snapshot snap = world.PackSnapshot();
    uint16_t size = sizeof(snap);
    SendToSlot(0, MsgID::S2C_SNAPSHOT, &snap, size);
    SendToSlot(1, MsgID::S2C_SNAPSHOT, &snap, size);
}

void Room::OnPlayerInput(int playerID, const InputState& input) {
    int slot = GetSlot(playerID);
    if (slot < 0 || state != RoomState::PLAYING) return;
    world.SetInput(slot, input);
}

void Room::OnPlayerShoot(int playerID) {
    int slot = GetSlot(playerID);
    if (slot < 0 || state != RoomState::PLAYING) return;
    world.RequestShoot(slot);
}

void Room::OnPlayerSkill(int playerID) {
    int slot = GetSlot(playerID);
    if (slot < 0 || state != RoomState::PLAYING) return;
    world.RequestSkill(slot);
}

void Room::OnHeartbeat(int playerID) {
    int slot = GetSlot(playerID);
    if (slot < 0) return;
    lastHeartbeatUs[slot] = Clock::Now();
}

void Room::HandleDisconnect(int playerID) {
    int slot = GetSlot(playerID);
    if (slot < 0) return;

    Logger::Get().Warn("玩家 " + playerNames[slot] +
                       " 断开连接，房间状态=" + std::to_string((int)state));

    if (state == RoomState::WAITING || state == RoomState::READY) {
        playerIDs[slot] = -1;
        playerNames[slot] = "";
        tankSelected[slot] = false;
        disconnectPending[slot] = false;
        if (state == RoomState::READY) state = RoomState::WAITING;
        Logger::Get().Game("玩家已从等待房间移除");
        return;
    }

    if (state == RoomState::PLAYING) {
        disconnectTimeUs[slot] = Clock::Now();
        disconnectPending[slot] = true; // re-set for timeout detection in CheckDisconnectTimeout
        state = RoomState::PAUSED;
        Logger::Get().Warn("游戏暂停，等待重连（" +
                           std::to_string(RECONNECT_MS/1000) + "秒）...");

        RoomState newState = RoomState::PAUSED;
        SendToSlot(0, MsgID::S2C_ROOM_INFO, &newState, sizeof(newState));
        SendToSlot(1, MsgID::S2C_ROOM_INFO, &newState, sizeof(newState));
        return;
    }

    if (state == RoomState::PAUSED) {
        disconnectTimeUs[slot] = Clock::Now();
        disconnectPending[slot] = true;
        Logger::Get().Warn("另一玩家也在暂停期间断开，等待重连...");
    }
}

bool Room::TryReconnect(const std::string& username, int newPlayerID) {
    for (int i = 0; i < 2; ++i) {
        if (playerNames[i] == username && disconnectPending[i]) {
            playerIDs[i] = newPlayerID;
            disconnectTimeUs[i] = 0;
            disconnectPending[i] = false;
            lastHeartbeatUs[i] = Clock::Now();

            Logger::Get().Game("玩家 " + username + " 重连成功");

            if (state == RoomState::PAUSED) {
                state = RoomState::PLAYING;

                // Notify both
                RoomState st = RoomState::PLAYING;
                SendToSlot(0, MsgID::S2C_ROOM_INFO, &st, sizeof(st));
                SendToSlot(1, MsgID::S2C_ROOM_INFO, &st, sizeof(st));
            }

            // Send full game state to reconnected player
            MatchResultData match;
            match.playerIDs[0] = playerIDs[0];
            match.playerIDs[1] = playerIDs[1];
            match.tankTypes[0] = world.tanks[0].type;
            match.tankTypes[1] = world.tanks[1].type;
            match.startPositions[0] = world.tanks[0].pos;
            match.startPositions[1] = world.tanks[1].pos;
            SendToSlot(i, MsgID::S2C_MATCH_RESULT, &match, sizeof(match));

            // Send current snapshot
            Snapshot snap = world.PackSnapshot();
            SendToSlot(i, MsgID::S2C_SNAPSHOT, &snap, sizeof(snap));

            return true;
        }
    }
    return false;
}

void Room::ForfeitPlayer(int playerID) {
    int slot = GetSlot(playerID);
    if (slot < 0) return;

    Logger::Get().Game("玩家 " + playerNames[slot] + " 断线超时，判负");
    int other = 1 - slot;
    EndGame(other, true);
}

void Room::CheckDisconnectTimeout(float /*dt*/) {
    int64_t now = Clock::Now();
    for (int i = 0; i < 2; ++i) {
        if (disconnectPending[i]) {
            int64_t elapsedUs = now - disconnectTimeUs[i];
            if (elapsedUs > RECONNECT_MS * 1000LL) {
                ForfeitPlayer(playerIDs[i]);
                return;
            }
        }
    }
}

void Room::EndGame(int winnerSlot, bool forfeit) {
    int loserSlot = 1 - winnerSlot;

    GameOverData data;
    data.winnerID = playerIDs[winnerSlot];
    data.forfeit  = forfeit;

    SendToSlot(0, MsgID::S2C_GAME_OVER, &data, sizeof(data));
    SendToSlot(1, MsgID::S2C_GAME_OVER, &data, sizeof(data));

    // 计算对局时长
    int durationSec = (int)((Clock::Now() - gameStartUs) / 1000000LL);

    // 保存对局记录
    GameRecord rec;
    strncpy(rec.player1, playerNames[0].c_str(), 31);
    rec.player1[31] = '\0';
    strncpy(rec.player2, playerNames[1].c_str(), 31);
    rec.player2[31] = '\0';
    strncpy(rec.winner, playerNames[winnerSlot].c_str(), 31);
    rec.winner[31] = '\0';
    rec.durationSec = durationSec;
    rec.kills[0] = world.kills[0];
    rec.kills[1] = world.kills[1];
    SaveGameRecord(rec);

    // 更新玩家战绩
    UpdateStats(playerNames[winnerSlot], 1, 0, world.kills[winnerSlot]);
    UpdateStats(playerNames[loserSlot], 0, 1, world.kills[loserSlot]);

    Logger::Get().Game("游戏结束！胜者=" + playerNames[winnerSlot] +
                       "，时长=" + std::to_string(durationSec) + "秒" +
                       (forfeit ? " (断线判负)" : ""));

    // Reset room
    playerIDs[0] = playerIDs[1] = -1;
    playerNames[0] = playerNames[1] = "";
    tankSelected[0] = tankSelected[1] = false;
    disconnectPending[0] = disconnectPending[1] = false;
    state = RoomState::WAITING;
    world.Reset();
}

void Room::SendToSlot(int slot, MsgID id, const void* body, uint16_t len) {
    if (slot < 0 || slot > 1) return;
    if (playerIDs[slot] == -1) return;
    if (sendToPlayer) {
        sendToPlayer(playerIDs[slot], id, body, len);
    }
}
