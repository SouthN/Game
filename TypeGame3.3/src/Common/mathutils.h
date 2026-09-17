/* ------------------------------------------------------------------
// 文件名     : mathutils.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 数学工具类，提供游戏内碰撞检测、随机数生成等基础计算支持
------------------------------------------------------------------ */
#pragma once
#include <QRectF>
#include <QPointF>
#include <cmath>
#include "commondefs.h"

namespace MathUtils
{
    inline qreal getDistance(const QPointF& p1, const QPointF& p2)
    {
        qreal dx = p2.x() - p1.x();
        qreal dy = p2.y() - p1.y();
        return std::sqrt(dx * dx + dy * dy);
    }

    inline QPointF getNormalizedDir(const QPointF& from, const QPointF& to)
    {
		QPointF dir = to - from; // 计算方向向量
        qreal length = getDistance(from, to);
        if (length < 1e-6) return QPointF(0, 0); //修改后：1e-6移除单精度后缀 f，匹配 qreal (double)
        return dir / length;
    }

    inline QRectF clampRectInParent(const QRectF& rect, const QRectF& parentRect)
    {
        QRectF result = rect;
        result.moveLeft(GameCommon::clampValue(result.left(), parentRect.left(), parentRect.right() - result.width()));
        result.moveTop(GameCommon::clampValue(result.top(), parentRect.top(), parentRect.bottom() - result.height()));
		return result; // 这个函数用于将一个矩形限制在父矩形内，避免超出边界
    }

    inline qreal lerp(qreal a, qreal b, qreal t)
    {
        return a + (b - a) * t; // 线性插值函数
    }
}