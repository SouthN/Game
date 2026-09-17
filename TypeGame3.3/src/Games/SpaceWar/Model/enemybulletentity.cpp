#include "enemybulletentity.h"
#include "resourcemanager.h"
#include "entityfactory.h"
#include <QPainter>
#include <QtMath>

EnemyBulletEntity::EnemyBulletEntity()
{
    // 构造时不分配具体大小，在 initBullet 中动态配置
}

void EnemyBulletEntity::initBullet(int bulletType, const QPointF& startPos, const QPointF& velocity, float damage)
{
    m_bulletType = bulletType;
    m_velocity = velocity;
    m_damage = damage;
    m_animationTimer = 0.0f;

    setPos(startPos);
    setAlive(true);
    setOpacity(1.0);
    setScale(1.0);

    QString imgPath;
    // 根据 Boss 类型加载不同的子弹素材和碰撞箱大小
    if (bulletType == 0) {
        // 地球军：传统穿甲弹 / 重机枪子弹
        imgPath = ":/SpaceWar/assets/images/SpaceWar/enemybullet1.png";
        m_width = 15;
        m_height = 80;
    }
    else if (bulletType == 1) {
        // 异虫巢穴：强酸毒液球 / 生物刺
        imgPath = ":/SpaceWar/assets/images/SpaceWar/enemybullet2.png";
        m_width = 80;
        m_height = 80;
    }
    else {
        // 泽塔星系：纯粹的能量光球
        imgPath = ":/SpaceWar/assets/images/SpaceWar/enemybullet3.png";
        m_width = 42;
        m_height = 42;
    }

    QPixmap pixmap = ResourceManager::instance()->loadPixmap(imgPath);
    if (!pixmap.isNull()) {
        m_texture = pixmap.scaled(m_width, m_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // 设置精准的碰撞包围盒及旋转中心
    m_rect = QRectF(0, 0, m_width, m_height);
    setBoundingRect(m_rect);
    setTransformOriginPoint(m_width / 2.0, m_height / 2.0);

    // 对于非圆形的子弹（如军方子弹），根据速度方向计算偏转角度
    // 假设素材原图是朝上的（Y轴负方向），我们需要旋转它
    if (bulletType == 0) {
        float angle = std::atan2(m_velocity.y(), m_velocity.x()) * 180.0 / M_PI;
        setRotation(angle - 90.0); // 调整素材朝向
    }
    else {
        setRotation(0); // 圆形毒液或能量球默认不依赖速度方向旋转
    }
}

void EnemyBulletEntity::updateLogic(float deltaTime)
{
    if (!isAlive()) return;

    prepareGeometryChange();

    // 基础的匀速直线运动
    setPos(x() + m_velocity.x() * deltaTime, y() + m_velocity.y() * deltaTime);

    // 对于圆形的子弹（虫族毒液 / 泽塔能量球），让它在飞行时产生自转效果，增加视觉张力
    if (m_bulletType == 1) {
        setRotation(rotation() + 360.0f * deltaTime); // 每秒转一圈
    }
    else if (m_bulletType == 2) {
        setRotation(rotation() - 180.0f * deltaTime); // 逆时针缓转
    }
}

void EnemyBulletEntity::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // 如果加载到了贴图则绘制贴图
    if (!m_texture.isNull()) {
        painter->save();
        painter->setCompositionMode(QPainter::CompositionMode_Plus);
        painter->drawPixmap(0, 0, m_texture);
        painter->restore();
    }
    // 兜底的几何图形渲染，防止素材没配置好时报错
    else {
        painter->setPen(Qt::NoPen);
        if (m_bulletType == 0) {
            painter->setBrush(QColor(255, 100, 50));
            painter->drawRect(m_rect);
        }
        else if (m_bulletType == 1) {
            painter->setBrush(QColor(100, 255, 50));
            painter->drawEllipse(m_rect);
        }
        else {
            painter->setBrush(QColor(50, 200, 255));
            painter->drawEllipse(m_rect);
        }
    }
}

void EnemyBulletEntity::destroy()
{
    // 利用你已经做好的对象池工厂回收它
    EntityFactory::instance()->destroyEntity<EnemyBulletEntity>(this);
}