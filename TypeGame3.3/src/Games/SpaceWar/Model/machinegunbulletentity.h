/* ------------------------------------------------------------------
// 文件名     : machinegunbulletentity.h
// 功能描述   : 直线机炮子弹实体，负责处理垂直向上的物理运动
------------------------------------------------------------------ */
#pragma once
#include "gameentity.h"
#include <QPixmap>
#include <QtGlobal>

class MachineGunBulletEntity : public GameEntity
{
    Q_OBJECT
public:
    // 根据 Bullet2.png 的比例设定的物理碰撞大小
    static constexpr int BULLET_WIDTH = 12;
    static constexpr int BULLET_HEIGHT = 72;

    MachineGunBulletEntity();
    ~MachineGunBulletEntity() override = default;

    void paint(QPainter* painter) override;
    void destroy() override;
    void updateLogic(float deltaTime = 0.0f) override;

    // 设置发射初始位置
    void setSpawnPosition(const QPointF& playerCenter);

    // 获取子弹伤害值
    float getDamage() const { return m_damage; }
    void setDamage(float damage) { m_damage = qMax(0.0f, damage); }

private:
    QPixmap m_texture;
    float m_speed = 1200.0f; // 极快的飞行速度
    float m_damage = 0.0f;        // 单发机炮伤害值，发射时由玩家属性组件注入
};
