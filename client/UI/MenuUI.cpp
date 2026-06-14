// MenuUI.cpp
#include "MenuUI.h"
#include "../Net/NetClient.h"
#include "../Net/MsgCodec.h"
#include <cstring>
#include <atomic>
#include <chrono>

// ===== 工具函数 =====
bool MenuUI::isMouseOver(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx <= x+w && my >= y && my <= y+h;
}

void MenuUI::drawButton(int x, int y, int w, int h,
                        const char* text, bool hovered) {
    setfillstyle(BS_SOLID);
    setfillcolor(hovered ? RGB(80,80,200) : RGB(50,50,150));
    setlinecolor(RGB(200,200,255));
    fillroundrect(x, y, x+w, y+h, 10, 10);

    settextcolor(WHITE);
    settextstyle(24, 0, "黑体");
    int tw = textwidth(text);
    int th = textheight(text);
    outtextxy(x + (w-tw)/2, y + (h-th)/2, text);
}

// ===== 主菜单 =====
MenuResult MenuUI::showMain() {
    int W = MapConfig::WIDTH, H = MapConfig::HEIGHT;
    int btnW = 200, btnH = 50;
    int btnX = (W - btnW) / 2;

    while (true) {
        MOUSEMSG m = {};
        bool hasMsg = MouseHit();

        // 获取鼠标位置
        int mx = 0, my = 0;
        if (hasMsg) {
            m = GetMouseMsg();
            mx = m.x; my = m.y;
        }

        // ===== 绘制 =====
        cleardevice();

        // 背景
        setbkcolor(RGB(20, 20, 40));
        cleardevice();

        // 标题
        settextcolor(RGB(255, 220, 50));
        settextstyle(48, 0, "黑体");
        outtextxy(W / 2 - 120, 120, "双人联机坦克大战");

        // 按钮
        bool hoverStart = isMouseOver(mx, my, btnX, 270, btnW, btnH);
        bool hoverRank  = isMouseOver(mx, my, btnX, 340, btnW, btnH);
        bool hoverExit  = isMouseOver(mx, my, btnX, 410, btnW, btnH);
        drawButton(btnX, 270, btnW, btnH, "开始游戏", hoverStart);
        drawButton(btnX, 340, btnW, btnH, "排 行 榜", hoverRank);
        drawButton(btnX, 410, btnW, btnH, "退    出", hoverExit);

        FlushBatchDraw();

        // ===== 事件 =====
        if (hasMsg && m.uMsg == WM_LBUTTONDOWN) {
            if (hoverStart) return MenuResult::START_GAME;
            if (hoverRank)  return MenuResult::LEADERBOARD;
            if (hoverExit)  return MenuResult::EXIT;
        }

        // ESC 退出
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            return MenuResult::EXIT;

        Sleep(16);
    }
}

// ===== 选坦克界面 =====
SelectResult MenuUI::showTankSelect() {
    int W = MapConfig::WIDTH, H = MapConfig::HEIGHT;

    struct TankOption {
        TankType type;
        const char* name;
        const char* desc;
        COLORREF color;
    };

    TankOption opts[3] = {
        { TankType::HEAVY, "重型坦克", "HP:200  速度:慢  伤害:大\n技能:铁壁护盾(3s免伤)", RGB(200,100,50) },
        { TankType::LIGHT, "轻型坦克", "HP:150  速度:中  伤害:中\n技能:涡轮冲刺(2s双倍速)", RGB(50,180,50) },
        { TankType::SCOUT, "侦察坦克", "HP:100  速度:快  伤害:小\n技能:弹幕散射(3颗扇形弹)", RGB(50,150,220) },
    };

    int selected = 1; // 默认选轻型

    while (true) {
        MOUSEMSG m = {};
        bool hasMsg = MouseHit();
        int mx = 0, my = 0;
        if (hasMsg) { m = GetMouseMsg(); mx = m.x; my = m.y; }

        cleardevice();
        setbkcolor(RGB(20, 20, 40));
        cleardevice();
        setfillstyle(BS_SOLID);  // 确保填充样式正常

        // 标题
        settextcolor(RGB(255,220,50));
        settextstyle(32, 0, "黑体");
        outtextxy(270, 40, "选择你的坦克");

        // 三个坦克卡片
        for (int i = 0; i < 3; i++) {
            int cx = 80 + i * 230;
            int cy = 120;
            int cw = 200, ch = 300;
            bool hover = isMouseOver(mx, my, cx, cy, cw, ch);
            bool isSel = (selected == i);

            // 卡片背景
            setfillcolor(isSel ? RGB(60,60,120) : (hover ? RGB(40,40,90) : RGB(30,30,60)));
            setlinecolor(isSel ? opts[i].color : RGB(80,80,80));
            setlinestyle(PS_SOLID, isSel ? 3 : 1);
            fillroundrect(cx, cy, cx+cw, cy+ch, 12, 12);

            // 坦克图标（简单画个矩形代替）
            setfillcolor(opts[i].color);
            fillrectangle(cx+60, cy+30, cx+140, cy+100);  // 车身
            fillrectangle(cx+80, cy+10, cx+120, cy+50);   // 炮塔
            fillrectangle(cx+108,cy+20, cx+128, cy+35);   // 炮管

            // 名字
            settextcolor(opts[i].color);
            settextstyle(22, 0, "黑体");
            int tw = textwidth(opts[i].name);
            outtextxy(cx + (cw-tw)/2, cy+115, opts[i].name);

            // 描述
            settextcolor(RGB(200,200,200));
            settextstyle(16, 0, "宋体");
            outtextxy(cx+10, cy+150, opts[i].desc);

            if (hover && hasMsg && m.uMsg == WM_LBUTTONDOWN)
                selected = i;
        }

        // 确认按钮
        bool hoverOK = isMouseOver(mx, my, 300, 470, 200, 50);
        drawButton(300, 470, 200, 50, "确认选择", hoverOK);

        // 提示
        settextcolor(RGB(150,150,150));
        settextstyle(16, 0, "宋体");
        outtextxy(280, 540, "ESC 返回主菜单");

        FlushBatchDraw();

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN && hoverOK)
            return { opts[selected].type, true };

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            return { TankType::LIGHT, false };

        Sleep(16);
    }
}

// ===== 排行榜 =====
void MenuUI::showLeaderboard(const char* serverIP) {
    int W = MapConfig::WIDTH, H = MapConfig::HEIGHT;

    // 连接服务器获取排行榜
    NetClient nc;
    if (!nc.connect(serverIP, SERVER_PORT)) {
        // 连接失败，显示错误后返回
        while (true) {
            cleardevice();
            setbkcolor(RGB(20, 20, 40));
            cleardevice();

            settextcolor(RGB(255, 100, 100));
            settextstyle(24, 0, "黑体");
            outtextxy(W / 2 - 150, H / 2 - 30, "无法连接到服务器");
            settextcolor(RGB(200, 200, 200));
            settextstyle(18, 0, "宋体");
            outtextxy(W / 2 - 130, H / 2 + 20, "按任意键返回主菜单");

            FlushBatchDraw();

            if (MouseHit()) { GetMouseMsg(); break; }
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
            if (GetAsyncKeyState(VK_RETURN) & 0x8000) break;

            Sleep(16);
        }
        return;
    }

    // 请求排行榜数据
    nc.sendMsg(MsgID::C2S_GET_RANK);

    // 等待服务器回复
    RankListData rankData{};
    std::atomic<bool> received{false};
    nc.setOnMsgCallback([&](MsgID id, const char* data, uint16_t len) {
        if (id == MsgID::S2C_RANK_LIST && len >= (int)sizeof(RankListData)) {
            memcpy(&rankData, data, sizeof(RankListData));
            received = true;
        }
    });

    auto startWait = std::chrono::high_resolution_clock::now();
    while (!received) {
        nc.poll();
        auto now = std::chrono::high_resolution_clock::now();
        float waitSec = std::chrono::duration<float>(now - startWait).count();
        if (waitSec > 3.0f) break;  // 超时 3 秒
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
        Sleep(10);
    }

    nc.disconnect();

    // 显示排行榜
    int btnW = 160, btnH = 40;
    int btnX = (W - btnW) / 2;
    int btnY = H - 100;

    while (true) {
        MOUSEMSG m = {};
        bool hasMsg = MouseHit();
        int mx = 0, my = 0;
        if (hasMsg) { m = GetMouseMsg(); mx = m.x; my = m.y; }

        cleardevice();
        setbkcolor(RGB(20, 20, 40));
        cleardevice();

        // 标题
        settextcolor(RGB(255, 220, 50));
        settextstyle(36, 0, "黑体");
        outtextxy(W / 2 - 80, 40, "排 行 榜");

        if (!received) {
            settextcolor(RGB(255, 150, 50));
            settextstyle(20, 0, "宋体");
            outtextxy(W / 2 - 100, H / 2, "获取数据超时，请稍后重试");
        } else if (rankData.count == 0) {
            settextcolor(RGB(200, 200, 200));
            settextstyle(22, 0, "宋体");
            outtextxy(W / 2 - 60, H / 2, "暂无数据");
        } else {
            // 表头
            settextcolor(RGB(200, 200, 200));
            settextstyle(20, 0, "黑体");
            int colX[5] = { 120, 260, 420, 560, 700 };
            outtextxy(colX[0], 110, "排名");
            outtextxy(colX[1], 110, "玩家");
            outtextxy(colX[2], 110, "胜场");
            outtextxy(colX[3], 110, "败场");
            outtextxy(colX[4], 110, "击杀");

            // 分隔线
            setlinecolor(RGB(100, 100, 150));
            setlinestyle(PS_SOLID, 1);
            line(100, 140, W - 100, 140);

            // 排行榜条目
            for (int i = 0; i < rankData.count; i++) {
                int y = 155 + i * 45;
                COLORREF rowColor = (i == 0) ? RGB(255, 215, 0) :
                                    (i == 1) ? RGB(192, 192, 192) :
                                    (i == 2) ? RGB(205, 127, 50) :
                                    RGB(200, 200, 200);

                settextcolor(rowColor);
                settextstyle(18, 0, "宋体");

                char rankStr[8];
                snprintf(rankStr, sizeof(rankStr), "#%d", i + 1);
                outtextxy(colX[0], y, rankStr);

                outtextxy(colX[1], y, rankData.entries[i].name);

                char winsStr[16], lossesStr[16], killsStr[16];
                snprintf(winsStr, sizeof(winsStr), "%d", rankData.entries[i].wins);
                snprintf(lossesStr, sizeof(lossesStr), "%d", rankData.entries[i].losses);
                snprintf(killsStr, sizeof(killsStr), "%d", rankData.entries[i].kills);
                outtextxy(colX[2], y, winsStr);
                outtextxy(colX[3], y, lossesStr);
                outtextxy(colX[4], y, killsStr);
            }
        }

        // 返回按钮
        bool hoverBack = isMouseOver(mx, my, btnX, btnY, btnW, btnH);
        drawButton(btnX, btnY, btnW, btnH, "返回主菜单", hoverBack);

        FlushBatchDraw();

        if (hasMsg && m.uMsg == WM_LBUTTONDOWN && hoverBack) return;
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return;

        Sleep(16);
    }
}
