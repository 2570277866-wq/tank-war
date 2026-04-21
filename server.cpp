// 包含Windows网络编程核心头文件
#include <WinSock2.h>
// 包含IP地址转换相关函数
#include <WS2tcpip.h>

// 多线程支持
#include <thread>
// 存储客户端socket
#include <vector>
// 互斥锁，保证线程安全
#include <mutex>
// 消息发送队列
#include <queue>
// 文件操作（保存用户数据）
#include <fstream>
#include <iostream>
#include <algorithm>
// SetConsoleOutputCP 函数
#include <Windows.h>

using namespace std;

// ===================== 全局配置 =====================
// 服务端监听端口号
const int PORT = 8888;
// 接收缓冲区大小
const int BUFFER_SIZE = 4096;
// 用户数据存储文件名（账号 密码 昵称）
const char* USER_FILE = "users.txt";

// ===================== 全局变量 =====================
// 互斥锁，保护客户端列表
mutex g_mutex;
// 存储所有已连接客户端的socket
vector<SOCKET> g_clients;

// ===================== 消息头结构体（用于消息分包） =====================
// 作用：解决TCP粘包问题，每个消息前都加一个头
struct MsgHeader {
    unsigned int msgId;    // 消息ID（用于区分消息类型）
    unsigned int msgLen;   // 消息体长度（用于分包解析）
};

// ===================== Session类：每个客户端连接一个对象 =====================
class Session {
public:
    SOCKET sock;                  // 客户端连接socket
    char recvBuf[BUFFER_SIZE];    // 接收缓冲区
    int recvLen;                  // 当前缓冲区有效数据长度

    queue<string> sendQueue;      // 待发送消息队列
    mutex sendMutex;              // 发送队列锁

    // 构造函数：初始化客户端连接
    Session(SOCKET s) : sock(s), recvLen(0) {
        memset(recvBuf, 0, BUFFER_SIZE); // 缓冲区清零
    }
};

// ===================== 处理解析完成的消息 =====================
void ProcessMsg(Session* session, const char* data, int len) {
    // 输出收到的消息（作业展示用）
    cout << "[服务端] 收到消息：" << data << endl;
}

// ===================== 客户端接收线程（每个客户端独立线程） =====================
void RecvThread(Session* session) {
    char tempBuf[BUFFER_SIZE];

    while (true) {
        // 接收客户端数据
        int ret = recv(session->sock, tempBuf, BUFFER_SIZE, 0);
        if (ret <= 0) break; // 连接断开则退出循环

        // 将收到的数据拷贝到会话缓冲区
        memcpy(session->recvBuf + session->recvLen, tempBuf, ret);
        session->recvLen += ret;

        // ===================== 消息分包处理（解决粘包） =====================
        while (session->recvLen >= (int)sizeof(MsgHeader)) {
            MsgHeader header;
            // 先拷贝出消息头
            memcpy(&header, session->recvBuf, sizeof(MsgHeader));

            // 计算一条完整消息总长度
            unsigned int totalLen = sizeof(MsgHeader) + header.msgLen;
            // 缓冲区数据不够一条完整消息，等待下一次接收
            if (session->recvLen < (int)totalLen) break;

            // ===================== 一条完整消息已收到 =====================
            // 交给业务逻辑处理
            ProcessMsg(session, session->recvBuf + sizeof(MsgHeader), header.msgLen);

            // 处理完一条消息，把缓冲区剩余数据前移
            session->recvLen -= (int)totalLen;
            memmove(session->recvBuf, session->recvBuf + totalLen, session->recvLen);
        }
    }

    // ===================== 客户端断开连接，清理资源 =====================
    closesocket(session->sock); // 关闭socket

    // 从全局客户端列表中移除
    lock_guard<mutex> lock(g_mutex);
    auto it = find(g_clients.begin(), g_clients.end(), session->sock);
    if (it != g_clients.end()) g_clients.erase(it);

    cout << "[服务端] 客户端断开连接" << endl;
    delete session; // 释放Session对象
}

// ===================== 接收客户端连接的循环（独立线程） =====================
void AcceptLoop(SOCKET listenSocket) {
    cout << "[服务端] 等待客户端连接..." << endl;

    while (true) {
        // 阻塞等待客户端连接
        SOCKET client = accept(listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        // 输出新客户端连接信息
        cout << "[服务端] 新客户端连接：socket = " << client << endl;

        // 将新客户端加入全局列表
        {
            lock_guard<mutex> lock(g_mutex);
            g_clients.push_back(client);
        }

        // 创建Session，为每个客户端启动独立接收线程
        Session* session = new Session(client);
        thread(RecvThread, session).detach(); // 分离线程
    }
}

// ===================== 保存用户数据到文件（账号 密码 昵称） =====================
void SaveUser(const string& account, const string& pwd, const string& name) {
    // 以追加模式打开文件
    ofstream ofs(USER_FILE, ios::app);
    if (ofs) {
        // 写入格式：账号 密码 昵称
        ofs << account << " " << pwd << " " << name << endl;
    }
}

// ===================== 主函数：服务端入口 =====================
int main() {
    //UTF-8 编码
    SetConsoleOutputCP(65001);
    // 1. 初始化Windows网络库 Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 2. 创建监听socket（流式TCP）
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 3. 绑定IP和端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);            // 端口号转网络字节序
    addr.sin_addr.s_addr = INADDR_ANY;      // 监听所有网卡

    bind(listenSocket, (sockaddr*)&addr, sizeof(addr));

    // 4. 开始监听，最大等待队列100
    listen(listenSocket, 100);

    // 5. 输出启动信息（作业展示用）
    cout << "=====================================" << endl;
    cout << "          TCP 服务端已启动" << endl;
    cout << "          监听端口：" << PORT << endl;
    cout << "=====================================" << endl;

    // 6. 启动独立线程处理连接，不阻塞主线程
    thread(AcceptLoop, listenSocket).detach();

    // 暂停，防止控制台一闪而过
    system("pause");
    return 0;
}