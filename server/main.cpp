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

static bool IsPrivateIPv4(uint32_t addr) {
    // RFC 1918 私有地址段: 10.x, 172.16-31.x, 192.168.x
    uint8_t b1 = (addr >> 0)  & 0xFF;
    uint8_t b2 = (addr >> 8)  & 0xFF;
    if (b1 == 10)   return true;
    if (b1 == 172 && b2 >= 16 && b2 <= 31) return true;
    if (b1 == 192 && b2 == 168) return true;
    return false;
}

static void PrintLocalIPs() {
    std::cout << "===== 本机局域网 IP 地址 =====" << std::endl;

    ULONG bufLen = 15000;
    std::vector<BYTE> buf(bufLen);
    PIP_ADAPTER_ADDRESSES adapters = (PIP_ADAPTER_ADDRESSES)buf.data();

    ULONG ret = GetAdaptersAddresses(AF_INET,
        GAA_FLAG_INCLUDE_PREFIX, nullptr, adapters, &bufLen);
    if (ret != NO_ERROR) return;

    int count = 0;
    const char* bestLabel = nullptr;
    char bestIP[16] = {};

    for (auto* a = adapters; a != nullptr; a = a->Next) {
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (a->OperStatus != IfOperStatusUp) continue;

        // 跳过常见的虚拟 / VPN 网卡
        if (a->FriendlyName) {
            std::wstring name(a->FriendlyName);
            std::wstring lower;
            for (wchar_t c : name) lower += towlower(c);
            if (lower.find(L"virtualbox")  != std::wstring::npos) continue;
            if (lower.find(L"hyper-v")     != std::wstring::npos) continue;
            if (lower.find(L"vmware")      != std::wstring::npos) continue;
            if (lower.find(L"bluetooth")   != std::wstring::npos) continue;
            if (lower.find(L"vpn")         != std::wstring::npos) continue;
            if (lower.find(L"tunnel")      != std::wstring::npos) continue;
        }

        for (auto* ua = a->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
            if (ua->Address.lpSockaddr->sa_family != AF_INET) continue;

            sockaddr_in* addr = (sockaddr_in*)ua->Address.lpSockaddr;
            char ipStr[16];
            inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));

            // 跳过 APIPA 自动分配地址（169.254.x.x）
            uint8_t b1 = (ntohl(addr->sin_addr.s_addr) >> 0) & 0xFF;
            uint8_t b2 = (ntohl(addr->sin_addr.s_addr) >> 8) & 0xFF;
            uint8_t b3 = (ntohl(addr->sin_addr.s_addr) >> 16) & 0xFF;
            if (b1 == 169 && b2 == 254) continue;
            // 跳过 VirtualBox Host-Only 默认网段 (192.168.56.x, 192.168.99.x)
            if (b1 == 192 && b2 == 168 && (b3 == 56 || b3 == 99)) continue;

            // 打印网卡名 + IP，方便识别
            const char* adapterDesc = "未知网卡";
            if (a->FriendlyName) {
                char nameBuf[256];
                WideCharToMultiByte(CP_UTF8, 0, a->FriendlyName, -1,
                                    nameBuf, sizeof(nameBuf), nullptr, nullptr);
                adapterDesc = nameBuf;
            }

            std::cout << "  [" << adapterDesc << "]" << std::endl;
            std::cout << "    IP: " << ipStr;

            // 优先推荐私有地址
            uint32_t hostAddr = ntohl(addr->sin_addr.s_addr);
            if (IsPrivateIPv4(hostAddr)) {
                std::cout << "  <-- 用这个";
                if (!bestLabel) {
                    bestLabel = "推荐";
                    strncpy(bestIP, ipStr, sizeof(bestIP) - 1);
                }
            }
            std::cout << std::endl;
            count++;
        }
    }

    if (count == 0) {
        std::cout << "  未检测到可用网卡，请检查网络连接" << std::endl;
    } else if (bestIP[0]) {
        std::cout << std::endl;
        std::cout << "  *** 客户端请填写: " << bestIP << " ***" << std::endl;
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
