#include "TCPServer.h"
#include "Session.h"
#include <iostream>

using namespace std;

TCPServer::~TCPServer()
{
    Shutdown();
}

bool TCPServer::Init()
{
    // 初始化Windows网络库 Winsock
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0)
    {
        cerr << "[服务端] WSAStartup 失败，错误码：" << err << endl;
        return false;
    }

    // 创建监听socket（流式TCP）
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        cerr << "[服务端] 创建 socket 失败，错误码：" << WSAGetLastError() << endl;
        WSACleanup();
        return false;
    }

    // 绑定IP和端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);       // 端口号转网络字节序
    addr.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡

    if (bind(listenSocket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        cerr << "[服务端] 绑定端口 " << PORT << " 失败，错误码：" << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    // 开始监听，最大等待队列100
    if (listen(listenSocket, 100) == SOCKET_ERROR)
    {
        cerr << "[服务端] 监听失败，错误码：" << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    // 输出启动信息（作业展示用）
    cout << "=====================================" << endl;
    cout << "          TCP 服务端已启动" << endl;
    cout << "          监听端口：" << PORT << endl;
    cout << "=====================================" << endl;

    return true;
}

void TCPServer::Start()
{
    running = true;
    // 启动独立线程处理连接，不阻塞主线程
    thread(&TCPServer::AcceptLoop, this).detach();
}

void TCPServer::Shutdown()
{
    if (!running.exchange(false))
        return; // 已经关闭过了

    closesocket(listenSocket);

    // 关闭所有客户端连接
    {
        lock_guard<mutex> lock(mtx);
        for (SOCKET s : clients)
            closesocket(s);
        clients.clear();
    }

    WSACleanup();
    cout << "[服务端] 服务端已停止" << endl;
}

void TCPServer::RemoveClient(SOCKET s)
{
    lock_guard<mutex> lock(mtx);
    auto it = find(clients.begin(), clients.end(), s);
    if (it != clients.end())
        clients.erase(it);
}

// 接收客户端连接的循环（独立线程）
void TCPServer::AcceptLoop()
{
    cout << "[服务端] 等待客户端连接..." << endl;

    while (running)
    {
        // 阻塞等待客户端连接（设置超时以便检查 running 标志）
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(listenSocket, &fds);

        timeval tv{1, 0}; // 1秒超时
        int ret = select(0, &fds, nullptr, nullptr, &tv);
        if (ret <= 0)
            continue;

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
        session->onDisconnect = [this, client]() {
            RemoveClient(client);
        };
        session->StartRecv();
    }
}
