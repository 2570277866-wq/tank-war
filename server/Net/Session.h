#pragma once

#include <WinSock2.h>
#include <queue>
#include <mutex>
#include <string> 
#include <thread> 
#include "Protocol.h"

using namespace std;

const int BUFFER_SIZE = 4096;

class Session
{
public:
    SOCKET sock;
    char recvBuf[BUFFER_SIZE];
    int recvLen;

    queue<string> sendQueue;
    mutex sendMutex;

public:
    Session(SOCKET s);
    void StartRecv();
};

void RecvThread(Session *session);
void ProcessMsg(Session *session, const char *data, int len);