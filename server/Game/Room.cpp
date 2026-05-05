#include "Room.h"

bool Room::Join(int playerID)
{
    for (int i = 0; i < 2; i++)
    {
        if (playerIDs[i] == -1)
        {
            playerIDs[i] = playerID;
            if (IsFull())
                state = RoomState::READY; // 满员，准备开始
            return true;
        }
    }
    return false; // 房间已满
}

void Room::Leave(int playerID)
{
    for (int i = 0; i < 2; i++)
    {
        if (playerIDs[i] == playerID)
        {
            playerIDs[i] = -1;
            state = RoomState::WAITING; // 有人离开，回到等待状态
            return;
        }
    }
}

bool Room::IsFull() const
{
    return playerIDs[0] != -1 && playerIDs[1] != -1;
}
