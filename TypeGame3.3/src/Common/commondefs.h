/* ------------------------------------------------------------------
// 文件名     : commondefs.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 公共宏定义与全局枚举，包含错误码及通用数据结构体
------------------------------------------------------------------ */
#pragma once
#include <QtGlobal>
#include <QRectF>
#include <QPointF>

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) if((p)) { delete (p); (p) = nullptr; } // 这个宏用于安全删除指针，避免重复删除和悬空指针问题
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) if((p)) { delete[] (p); (p) = nullptr; } // 这个宏用于安全删除数组指针，避免重复删除和悬空指针问题
#endif

namespace GameCommon
{
    constexpr int DEFAULT_SCREEN_WIDTH = 800;
    constexpr int DEFAULT_SCREEN_HEIGHT = 600;
    constexpr int MIN_LEVEL = 1;
    constexpr int MAX_LEVEL = 10;
    constexpr char MIN_LETTER = 'A';
    constexpr char MAX_LETTER = 'Z';
    constexpr int LETTER_COUNT = 26;

    inline bool checkRectCollision(const QRectF& rect1, const QRectF& rect2)
    {
        return rect1.intersects(rect2); 
    }

    inline qreal clampValue(qreal value, qreal min, qreal max)
    {
        return qBound(min, value, max); // 这个函数用于将值限制在指定范围内
    }
}