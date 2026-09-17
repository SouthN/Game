/* ------------------------------------------------------------------
// 文件名     : enemybulletentity.h
// 功能描述   : 敌方子弹实体，用于 Boss 或未来特殊敌机向玩家射击
------------------------------------------------------------------ */
#pragma once
#include "gameentity.h"
#include <QPixmap>
#include <QPointF>

class EnemyBulletEntity : public GameEntity
{
    Q_OBJECT
public:
    EnemyBulletEntity();
    ~EnemyBulletEntity() override = default;

    // 初始化子弹状态
    // bulletType: 0=军方机炮, 1=虫族酸液, 2=外星能量球
    void initBullet(int bulletType, const QPointF& startPos, const QPointF& velocity, float damage = 1.0f);

    void paint(QPainter* painter) override;
    void updateLogic(float deltaTime = 0.0f) override;
    void destroy() override;

    float getDamage() const { return m_damage; }
    int getBulletType() const { return m_bulletType; }

private:
    int m_bulletType = 0;
    QPointF m_velocity;    // 子弹的二维速度向量
    float m_damage = 1.0f; // 子弹造成的伤害

    QPixmap m_texture;     // 子弹贴图
    int m_width = 15;      // 动态宽度
    int m_height = 30;     // 动态高度

    // 动画状态（用于旋转或动效）
    float m_animationTimer = 0.0f;
};