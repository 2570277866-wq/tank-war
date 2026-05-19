#pragma once

#include <string>

struct GameRecord;

// 保存一局游戏记录到 records.dat
bool SaveGameRecord(const GameRecord& rec);
