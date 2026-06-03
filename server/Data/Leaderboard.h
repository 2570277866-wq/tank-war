#pragma once

#include "../../Common/Protocol.h"

// 获取排行榜前 N 名（按胜率降序，胜率相同按击杀数降序）
RankListData GetTopPlayers(int topN = 10);
