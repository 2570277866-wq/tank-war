# 双人联机坦克大战

C++ 双人联机对战坦克游戏。客户端-服务端（C/S）架构，服务端权威判定，支持实时状态同步。

## 功能特性

- 🎮 3 种坦克（重型/轻型/侦察），各有独特技能
- 🌐 TCP 联机对战，服务端权威判定
- 🗺️ 交互式地图（砖墙可破坏、钢墙不可破坏、草丛隐身、水域阻隔）
- 💥 粒子特效系统（爆炸、伤害飘字、护盾光环、冲刺拖尾）
- 🏆 战绩记录 & 排行榜
- 🔄 客户端状态插值平滑
- ⚡ 断线重连

## 环境要求

| 依赖 | 说明 |
|------|------|
| **操作系统** | Windows 10/11 |
| **编译器** | MSYS2 UCRT64 的 `g++`（推荐）或 MSVC 2022 |
| **CMake** | ≥ 3.15 |
| **EasyX** | 图形库，需手动安装到 MSYS2 |

## 环境搭建

### 1. 安装 MSYS2

从 [msys2.org](https://www.msys2.org/) 下载安装器，安装到默认路径 `C:\msys64`。

安装完成后打开 **UCRT64 终端**（开始菜单搜索 "UCRT64"），执行：

```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make
```

### 2. 安装 EasyX

EasyX 是为 MSVC 设计的图形库，在 MinGW 下使用需要手动安装头文件。

1. 从 [easyx.cn](https://easyx.cn/) 下载 EasyX，安装到任意目录
2. 找到安装目录下的 `include/easyx.h` 和 `include/graphics.h`，复制到：
   ```
   C:\msys64\ucrt64\include\
   ```
3. 在安装目录找到 `lib\VC2022\EasyXa.lib`（或对应版本），复制到：
   ```
   C:\msys64\ucrt64\lib\libeasyx.a
   ```
   > **注意**：MinGW 的 `-leasyx` 会搜索 `libeasyx.a`，所以必须把 `.lib` 重命名为 `libeasyx.a`。

   如果没有 EasyXa.lib，也可以用 `gendef` + `dlltool` 从 `EasyXa.dll` 生成：
   ```bash
   gendef EasyXa.dll
   dlltool -d EasyXa.def -l libeasyx.a
   cp libeasyx.a /c/msys64/ucrt64/lib/
   ```

4. 验证安装：
   ```bash
   ls C:/msys64/ucrt64/include/graphics.h   # 应存在
   ls C:/msys64/ucrt64/include/easyx.h      # 应存在
   ls C:/msys64/ucrt64/lib/libeasyx.a       # 应存在
   ```

## 构建

在 **UCRT64 终端**中进入项目目录：

```bash
cd tank-war
cmake -B build -G "MSYS Makefiles"
cmake --build build -j$(nproc)
```

构建产物：
- `build/tank-war.exe` — 客户端
- `build/tank_server.exe` — 服务端

### MSVC 构建（备选）

```powershell
cmake -B build
cmake --build build --config Release
```

> 使用 MSVC 时不需要 EasyX 手动安装（有官方安装器），也不需要 `iob_shim.c`。

## 运行

### 服务端

先启动服务端（对局双方都需要连接到同一服务端）：

```bash
cd build
./tank_server.exe
```

服务端启动后会打印本机局域网 IP，客户端需要填入这个 IP。默认监听 `9527` 端口，也可指定端口：

```bash
./tank_server.exe 8888
```

### 客户端

在 **同一台或另一台电脑**上启动客户端：

```bash
cd build
./tank-war.exe
```

1. 主菜单选择「开始游戏」
2. 输入**服务端 IP**（本机用 `127.0.0.1`）和用户名密码
3. 选择坦克类型
4. 等待对手加入后自动开战

> 服务端 IP 会自动保存到 `server_ip.cfg`，下次启动无需重新输入。

## 操作说明

| 按键 | 操作 |
|------|------|
| `W` `A` `S` `D` | 移动 / 转向 |
| `空格` | 射击 |
| `F` | 释放技能 |
| `Esc` | 取消 / 返回 |

### 坦克类型

| 类型 | 血量 | 速度 | 伤害 | 技能 |
|------|------|------|------|------|
| 🛡️ 重型坦克 | 200 | 慢 | 30 | 铁壁护盾（3 秒免伤，CD 15s） |
| ⚖️ 轻型坦克 | 150 | 中 | 20 | 涡轮冲刺（2 秒双倍速，CD 12s） |
| 🎯 侦察坦克 | 100 | 快 | 15 | 弹幕散射（扇形 3 弹，CD 10s） |

## 目录结构

```
tank-war/
├── client/              # 客户端代码
│   ├── main.cpp
│   ├── UI/              # 界面（菜单/登录/等待/结算/小地图）
│   ├── Entity/          # 实体（坦克/子弹）
│   ├── Core/            # 核心（游戏循环/输入/状态插值）
│   ├── Effect/          # 特效（粒子/碰撞/地图/死亡动画）
│   └── Net/             # 网络（TCP 客户端/消息编解码）
├── server/              # 服务端代码
│   ├── main.cpp
│   ├── Net/             # 网络（TCP 服务端/Session）
│   ├── Game/            # 游戏逻辑（房间/世界/碰撞判定）
│   └── Data/            # 数据（用户/战绩/排行榜）
├── Common/              # 共享协议（消息定义/配置/地图数据）
└── CMakeLists.txt
```

## 常见问题

### Q: 编译报 `fatal error: graphics.h: No such file or directory`
EasyX 头文件未安装或路径不对，检查 `C:/msys64/ucrt64/include/` 下是否有 `graphics.h`。

### Q: 链接报 `undefined reference to __imp___iob_func`
EasyX 版本与 UCRT 不兼容。本项目已内置兼容 shim（`client/iob_shim.c`），如果仍报错，说明 EasyX 库版本过旧，请从官网下载最新版。

### Q: 客户端连接服务端超时
- 检查服务端是否已启动
- 检查 IP 地址是否填写正确（局域网用服务端打印的 IP）
- 检查防火墙是否放行 `9527` 端口

### Q: 两个客户端都是本机，如何联机测试？
启动一个服务端 + 两个客户端，两个客户端都连接 `127.0.0.1` 即可。
