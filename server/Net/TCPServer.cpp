#include "TCPServer.h"
#include "Session.h"
#include <iostream>
#include <cstring>

using namespace std;

bool TCPServer::Init()
{
    // 初始化Windows网络库 Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 创建监听socket（流式TCP）
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //  绑定IP和端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);       // 端口号转网络字节序
    addr.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡

    bind(listenSocket, (sockaddr *)&addr, sizeof(addr));

    // 开始监听，最大等待队列100
    listen(listenSocket, 100);

    // 输出启动信息（作业展示用）
    cout << "=====================================" << endl;
    cout << "          TCP 服务端已启动" << endl;
    cout << "          监听端口：" << PORT << endl;
    cout << "=====================================" << endl;

    return true;
}

void TCPServer::Start()
{
    // 启动独立线程处理连接，不阻塞主线程
    thread(&TCPServer::AcceptLoop, this).detach();
}

// 接收客户端连接的循环（独立线程）
void TCPServer::AcceptLoop()
{
    cout << "[服务端] 等待客户端连接..." << endl;

    while (true)
    {
        // 阻塞等待客户端连接
        SOCKET client = accept(listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET)
            continue;

        // 输出新客户端连接信息
        cout << "[服务端] 新客户端连接：socket = " << client << endl;

        // 将新客户端加入全局列表
        {
            lock_guard<mutex> lock(mtx);
            clients.push_back(client);
        }

        // 创建Session，为每个客户端启动独立接收线程
        Session *session = new Session(client);
        session->StartRecv();
    }
}