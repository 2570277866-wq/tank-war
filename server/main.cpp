// 主函数：服务端入口
#include "Net/TCPServer.h"
#include "../Common/Protocol.h"
#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <cstdlib>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")

static void PrintLocalIPs() {
    std::cout << "===== 本机局域网 IP 地址 =====" << std::endl;

    ULONG bufLen = 15000;
    std::vector<BYTE> buf(bufLen);
    PIP_ADAPTER_ADDRESSES adapters = (PIP_ADAPTER_ADDRESSES)buf.data();

    ULONG ret = GetAdaptersAddresses(AF_INET,
        GAA_FLAG_INCLUDE_PREFIX, nullptr, adapters, &bufLen);
    if (ret != NO_ERROR) return;

    int count = 0;
    for (auto* a = adapters; a != nullptr; a = a->Next) {
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (a->OperStatus != IfOperStatusUp) continue;

        for (auto* ua = a->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
            if (ua->Address.lpSockaddr->sa_family != AF_INET) continue;

            sockaddr_in* addr = (sockaddr_in*)ua->Address.lpSockaddr;
            char ipStr[16];
            inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));

            std::cout << "  " << ipStr << std::endl;
            count++;
        }
    }

    if (count == 0) {
        std::cout << "  未检测到可用网卡，请检查网络连接" << std::endl;
    }
    std::cout << "===============================" << std::endl << std::endl;
}

int main(int argc, char* argv[])
{
    // UTF-8 编码，解决中文乱码
    SetConsoleOutputCP(65001);

    // 解析命令行参数：tank_server.exe [端口号]
    uint16_t port = SERVER_PORT;
    if (argc >= 2) {
        int p = std::atoi(argv[1]);
        if (p > 0 && p <= 65535) {
            port = static_cast<uint16_t>(p);
        } else {
            std::cerr << "无效端口号：" << argv[1] << "，使用默认端口 " << SERVER_PORT << std::endl;
        }
    }

    PrintLocalIPs();

    std::cout << "启动服务端，端口：" << port << std::endl;

    TCPServer server;
    if (!server.Init(port))
    {
        std::cerr << "服务端初始化失败！" << std::endl;
        system("pause");
        return 1;
    }

    server.Start(); // 启动监听

    std::cout << "按回车键停止服务端..." << std::endl;
    std::cin.get();

    server.Shutdown();
    return 0;
}
