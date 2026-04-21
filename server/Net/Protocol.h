#pragma once

// ===================== 消息头结构体（用于消息分包） =====================
// 作用：解决TCP粘包问题，每个消息前都加一个头
struct MsgHeader
{
    unsigned int msgId;  // 消息ID（用于区分消息类型）
    unsigned int msgLen; // 消息体长度（用于分包解析）
};