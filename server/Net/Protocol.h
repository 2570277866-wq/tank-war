#pragma once

// 消息头结构体，解决TCP粘包问题
struct MsgHeader
{
    unsigned int msgId;  // 消息ID（用于区分消息类型）
    unsigned int msgLen; // 消息体长度（用于分包解析）
};