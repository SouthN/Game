#include "appleentity.h"
#include "resourcemanager.h"
#include "entityfactory.h" // 新增：引入工厂头文件
#include <QPainterPath>

AppleEntity::AppleEntity()
{
    // 初始化自身特有的尺寸和初始速度
    m_rect = QRectF(0, 0, APPLE_WIDTH, APPLE_HEIGHT);
    setBoundingRect(m_rect);
    m_fallSpeed = AppleGameConfig::instance()->getFallSpeed();
}

void AppleEntity::destroy()
{
    // 修复：将对象交还给内存池回收，绝对不能调用 delete 或 deleteLater
    EntityFactory::instance()->destroyEntity<AppleEntity>(this);
}

void AppleEntity::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    // m_opacity, m_scaleX, m_scaleY, m_flash 这些变量已经全部在基类 GameEntity 声明为了 protected 成员
    painter->setOpacity(m_opacity);

    painter->save();

    QRectF bounds = boundingRect();
    QPointF center = bounds.center();
    painter->translate(center);
    painter->scale(m_scaleX, m_scaleY); // 应用基类的形变属性
    painter->translate(-center);

    QString imgPath = (!isAlive())
        ? ":/SaveTheApple/assets/images/SaveTheApple/APPLE_BAD2.png"
        : ":/SaveTheApple/assets/images/SaveTheApple/APPLE_REAL.png";
    QPixmap applePixmap = ResourceManager::instance()->loadPixmap(imgPath);

    if (applePixmap.isNull()) {
        painter->setBrush(!isAlive() ? Qt::red : Qt::green);
        painter->drawRoundedRect(bounds, 10, 10);
    }
    else {
        QPixmap scaledPixmap = applePixmap.scaled(
            bounds.size().toSize(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        QPointF topLeft = bounds.center() - QPointF(scaledPixmap.width() / 2, scaledPixmap.height() / 2);
        painter->drawPixmap(topLeft, scaledPixmap);
    }

    if (m_flash > 0.0) {
        painter->save();
        painter->setOpacity(m_flash * 0.9);
        painter->setBrush(Qt::white);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(bounds);
        painter->restore();
    }

    if (isAlive()) {
        QFont font = painter->font();
        font.setPixelSize(24);
        font.setBold(true);
        painter->setFont(font);

        QPainterPath textPath;
        textPath.addText(bounds.center(), font, m_letter);
        QRectF textRect = textPath.boundingRect();
        textPath.translate(bounds.center() - textRect.center());
        painter->strokePath(textPath, QPen(Qt::black, 2));
        painter->fillPath(textPath, Qt::white);
    }

    painter->restore();
}
// 【注】动画播放、透明度Getter/Setter、选中逻辑、updateFall 的代码全部被删除了，因为基类已经做好了！