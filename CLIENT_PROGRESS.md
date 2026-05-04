# 客户端开发进度 (同学A)

## 项目结构

```
client/
├── main.cpp                  # 主入口，流程控制
├── Core/                     # 核心逻辑
│   ├── GameLoop.h/cpp        # 游戏主循环：更新+渲染+碰撞+网络上报
│   ├── InputHandler.h/cpp    # 键盘输入：GetAsyncKeyState读取WASD/空格/F
│   └── StateInterpolator.h/cpp # 客户端插值平滑：位置/角度线性插值
├── Entity/                   # 实体类
│   ├── Tank.h/cpp            # 坦克基类 + HeavyTank/LightTank/ScoutTank
│   └── Bullet.h/cpp          # 子弹：飞行+出界销毁
├── Effect/                   # 特效与碰撞
│   ├── Collision.h/cpp       # 碰撞检测：圆形碰撞
│   ├── ParticleSystem.h/cpp  # 粒子系统：爆炸/伤害飘字/护盾/冲刺拖尾
│   ├── GameMap.h/cpp         # 地图：砖墙(可破坏)/钢墙/草丛(隐身)/水域
│   └── DeathAnimator.h/cpp   # 死亡动画：扩散环+碎片
├── Net/                      # 网络层
│   ├── NetClient.h/cpp       # TCP客户端：连接/收发线程/消息分包
│   └── MsgCodec.h            # 消息编解码：登录/输入/射击/技能/房间/排行榜
└── UI/                       # 界面
    ├── MenuUI.h/cpp          # 主菜单 + 坦克选择
    ├── LoginUI.h/cpp         # 登录/注册界面
    ├── WaitUI.h/cpp          # 等待匹配界面
    ├── Minimap.h/cpp         # 小地图
    └── GameOverUI.h/cpp      # 结算界面
```

## 开发进度

### W1 基础搭建 ✅

| 提交 | 内容 |
|------|------|
| `13d0a7f` | 初始化项目骨架：Protocol.h + Config.h + 目录结构 |
| `14557b6` | Tank/Bullet/GameLoop逻辑骨架 |
| `e180841` | EasyX主菜单 + 选坦克界面 |

### W2 登录注册 + 坦克类体系 ✅

| 提交 | 内容 |
|------|------|
| `ca652d7` | EasyX渲染/键盘输入/射击技能/HUD，修复MapConfig冲突和子弹速度 |
| `5585b5b` | 修复技能计时器(护盾/冲刺到期自动关闭) |
| `386992a` | 登录注册界面(LoginUI) - 输入框/Tab切换/密码星号/光标闪烁 |
| `f3f6e26` | NetClient网络层 + MsgCodec消息编解码 + 接入GameLoop |

### W3 单人本地可玩 ✅

| 提交 | 内容 |
|------|------|
| `cddcb6f` | 粒子特效系统 - 爆炸粒子/伤害飘字/护盾粒子/冲刺拖尾 |

### W4 联机打通 + 技能系统 ✅

| 提交 | 内容 |
|------|------|
| `841c3fe` | 快照同步 + 客户端预测 - applySnapshot/服务端修正/NetClient回调 |

### W5 地图完善 + 战绩系统 ✅

| 提交 | 内容 |
|------|------|
| `31c45f1` | 地图系统 - 砖墙(可破坏)/钢墙/草丛(隐身)/水域 + 墙壁碰撞 |
| `116a8f4` | 等待房间界面 - 匹配动画/取消匹配 |
| `ba2cc2c` | 死亡动画/小地图/结算界面 - 爆炸扩散环+碎片/缩略位置/战绩统计 |

### W6 体验打磨 ✅

| 提交 | 内容 |
|------|------|
| `f5c9ee3` | 客户端插值平滑 - 位置/角度线性插值 + 角度环绕处理 |

---

## 客户端完整流程

```
启动 → 主菜单(MenuUI)
     → 登录/注册(LoginUI)
     → 选坦克(MenuUI::showTankSelect)
     → 连接服务端(NetClient::connect)
     → 发送登录/注册消息
     → 发送加入房间消息
     → 等待匹配(WaitUI)
     → 游戏循环(GameLoop)
         ├─ 每帧: 读取键盘 → 更新坦克 → 射击/技能 → 碰撞检测 → 渲染
         ├─ 网络: 上报输入(C2S_INPUT) + 射击(C2S_SHOOT) + 技能(C2S_USE_SKILL)
         └─ 接收: 快照同步(S2C_SNAPSHOT) → applySnapshot → 服务端修正
     → 结算(GameOverUI)
     → 返回主菜单
```

## 网络消息交互

| 方向 | 消息 | 触发时机 |
|------|------|---------|
| C→S | C2S_LOGIN / C2S_REGISTER | 登录/注册界面提交 |
| C→S | C2S_JOIN_ROOM | 选完坦克后 |
| C→S | C2S_INPUT | 每帧上报按键状态 |
| C→S | C2S_SHOOT | 空格按下时 |
| C→S | C2S_USE_SKILL | F键按下时 |
| C→S | C2S_HEARTBEAT | 定时发送 |
| S→C | S2C_LOGIN_ACK / S2C_REGISTER_ACK | 登录/注册结果 |
| S→C | S2C_MATCH_RESULT | 匹配成功 |
| S→C | S2C_SNAPSHOT | 服务端每50ms广播 |
| S→C | S2C_HIT | 命中判定 |
| S→C | S2C_SKILL_EFFECT | 技能效果 |
| S→C | S2C_GAME_OVER | 游戏结束 |
| S→C | S2C_RANK_LIST | 排行榜数据 |

## 待办事项

- [ ] 在 Windows + EasyX 环境编译验证
- [ ] 联调登录流程（需同学B修好协议后）
- [ ] 联调快照同步（需服务端GameWorld完成）
- [ ] 加入音效（射击/爆炸/技能）
- [ ] 加入敌方AI（单人模式）
- [ ] 帧率自适应（替代固定Sleep(16)）
- [ ] 窗口大小适配
