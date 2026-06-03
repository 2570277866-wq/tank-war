#include "Leaderboard.h"
#include "UserManager.h"
#include <algorithm>
#include <cstring>

RankListData GetTopPlayers(int topN)
{
    RankListData result;
    result.count = 0;

    auto users = GetAllUsers();

    std::sort(users.begin(), users.end(),
        [](const UserStats& a, const UserStats& b) {
            int totalA = a.wins + a.losses;
            int totalB = b.wins + b.losses;
            float rateA = totalA > 0 ? (float)a.wins / totalA : 0.0f;
            float rateB = totalB > 0 ? (float)b.wins / totalB : 0.0f;

            if (rateA != rateB) return rateA > rateB;
            return a.kills > b.kills;
        });

    int n = std::min(topN, (int)users.size());
    result.count = n;
    for (int i = 0; i < n; ++i) {
        strncpy(result.entries[i].name, users[i].account.c_str(), 31);
        result.entries[i].name[31] = '\0';
        result.entries[i].wins   = users[i].wins;
        result.entries[i].losses = users[i].losses;
        result.entries[i].kills  = users[i].kills;
    }

    return result;
}
