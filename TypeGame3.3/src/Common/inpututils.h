/* ------------------------------------------------------------------
// 文件名     : inpututils.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 处理输入字母
------------------------------------------------------------------ */
#pragma once
#include <QKeyEvent>
#include <QChar>
#include "commondefs.h"

namespace InputUtils 
{
    // 这个函数用于从键盘事件中获取大写字母
    inline char getUpperLetterFromKeyEvent(QKeyEvent* event)
    {
        if (event->text().isEmpty()) return 0;
        QChar ch = event->text().toUpper().at(0);
        if (ch < GameCommon::MIN_LETTER || ch > GameCommon::MAX_LETTER) return 0;
        return ch.toLatin1(); 
    }

    inline bool isUpperLetter(char ch)
    {
        return ch >= GameCommon::MIN_LETTER && ch <= GameCommon::MAX_LETTER;
    }

    inline bool isLetter(char ch)
    {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    }
}