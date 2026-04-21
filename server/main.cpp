// 主函数：服务端入口
#include "Net/TCPServer.h"
#include <Windows.h>

int main()
{
    // UTF-8 编码，解决中文乱码
    SetConsoleOutputCP(65001);

    TCPServer server;
    server.Init();  // 初始化服务端
    server.Start(); // 启动监听

    // 暂停，防止控制台一闪而过
    system("pause");
    return 0;
}