#include "RecordManager.h"
#include "../../Common/Protocol.h"
#include <fstream>
#include <ctime>

using namespace std;

static const char* RECORDS_FILE = "records.dat";

bool SaveGameRecord(const GameRecord& rec)
{
    ofstream ofs(RECORDS_FILE, ios::app);
    if (!ofs) return false;

    time_t now = time(nullptr);
    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    ofs << timeBuf << "|"
        << rec.player1 << "|"
        << rec.player2 << "|"
        << rec.winner << "|"
        << rec.durationSec << "|"
        << rec.kills[0] << "|"
        << rec.kills[1] << endl;

    return true;
}
