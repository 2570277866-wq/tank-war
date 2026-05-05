// 主函数：服务端入口
#include "Net/TCPServer.h"
#include <Windows.h>
#include <iostream>

int main()
{
    // UTF-8 编码，解决中文乱码
    SetConsoleOutputCP(65001);

    TCPServer server;
    if (!server.Init())
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
