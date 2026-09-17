#include "machinegunbulletentity.h"
#include "resourcemanager.h"
#include "entityfactory.h"

MachineGunBulletEntity::MachineGunBulletEntity()
{
    m_rect = QRectF(0, 0, BULLET_WIDTH, BULLET_HEIGHT);
    setBoundingRect(m_rect);
    setTransformOriginPoint(BULLET_WIDTH / 2.0, BULLET_HEIGHT / 2.0);

    // 加载直线子弹素材
    m_texture = ResourceManager::instance()->loadPixmap(":/SpaceWar/assets/images/SpaceWar/Bullet2.png");
}

void MachineGunBulletEntity::setSpawnPosition(const QPointF& playerCenter)
{
    // 从战机正中心机头位置发射
    float startX = playerCenter.x() - BULLET_WIDTH / 2.0;
    float startY = playerCenter.y() - BULLET_HEIGHT;
    setPos(startX, startY);
}

void MachineGunBulletEntity::updateLogic(float deltaTime)
{
    // 只有存活状态才更新物理位置
    if (!isAlive()) return;

    // 通知 QGraphicsScene 即将改变实体局部包围盒位置
    prepareGeometryChange();

    // 每一帧单纯地向上做匀速直线运动 (Y轴减小)
    setPos(x(), y() - m_speed * deltaTime);

    // 【自身越界回收机制】：如果子弹完全飞出屏幕上方，则标记为死亡
    // Controller 会在遍历时将其从场景中剔除并回收
    if (y() + BULLET_HEIGHT < 0) {
        setAlive(false);
    }
}

void MachineGunBulletEntity::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing);

    if (!m_texture.isNull()) {
        painter->save();
        // ================= 【核心修改：加法混合过滤黑底】 =================
        // CompositionMode_Plus 会将像素颜色值相加，完美忽略纯黑背景，且自带发光增强效果
        painter->setCompositionMode(QPainter::CompositionMode_Plus);
        // =================================================================
        painter->drawPixmap(0, 0, BULLET_WIDTH, BULLET_HEIGHT, m_texture);
        painter->restore();
    }
    else {
        // 兜底色块：如果图片没加载成功，画一个耀眼的激光黄条
        painter->setBrush(QColor(255, 255, 0));
        painter->setPen(Qt::NoPen);
        painter->drawRect(0, 0, BULLET_WIDTH, BULLET_HEIGHT);
    }
}

void MachineGunBulletEntity::destroy()
{
    // 通过工厂回收对象，进入对象池待复用，避免频繁 new/delete 造成的内存碎片
    EntityFactory::instance()->destroyEntity<MachineGunBulletEntity>(this);
}