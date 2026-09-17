#include "gameentity.h"

GameEntity::GameEntity()
{
    setFlag(ItemIsSelectable, false);

    m_fadeInAnim = new QPropertyAnimation(this, "opacity", this);
    m_fadeInAnim->setDuration(300);
    m_fadeInAnim->setStartValue(0.0);
    m_fadeInAnim->setEndValue(1.0);
    m_fadeInAnim->setEasingCurve(QEasingCurve::InOutQuad);

    m_fadeOutAnim = new QPropertyAnimation(this, "opacity", this);
    m_fadeOutAnim->setDuration(300);
    m_fadeOutAnim->setStartValue(1.0);
    m_fadeOutAnim->setEndValue(0.0);
    m_fadeOutAnim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_fadeOutAnim, &QPropertyAnimation::finished, this, &GameEntity::fadeOutFinished);
}

void GameEntity::setSelected(bool selected)
{
    if (m_isSelected == selected) return;
    m_isSelected = selected;
    update();
}

void GameEntity::updateLogic(float deltaTime)
{
    if (m_isSelected || !isAlive()) return;
    // 物理与帧率解耦的统一位移计算
    float actualSpeed = m_fallSpeed * 60.0f * deltaTime;
	setPos(x(), y() + actualSpeed); // 只更新位置，绘制逻辑由 paint() 负责
}

qreal GameEntity::opacity() const { return m_opacity; }

void GameEntity::setOpacity(qreal opacity)
{
    if (qFuzzyCompare(m_opacity, opacity)) return;
    m_opacity = qBound(0.0, opacity, 1.0);
    update();
    emit opacityChanged();
}

void GameEntity::startFadeIn(int duration)
{
    m_fadeOutAnim->stop();
    m_fadeInAnim->setDuration(duration);
    m_fadeInAnim->start();
}

void GameEntity::startFadeOut(int duration)
{
    m_fadeInAnim->stop();
    m_fadeOutAnim->setDuration(duration);
    m_fadeOutAnim->start();
}

void GameEntity::startSelectedAnimation()
{
    setOpacity(1.0);
    setScaleX(1.0);
    setScaleY(1.0);
    setFlash(0.0);

    QSequentialAnimationGroup* scaleSeq = new QSequentialAnimationGroup(this);

    // 蓄力
    QPropertyAnimation* anticipate = new QPropertyAnimation(this, "");
    anticipate->setDuration(100);
    anticipate->setStartValue(1.0);
    anticipate->setEndValue(0.8);
    connect(anticipate, &QPropertyAnimation::valueChanged, this, [this](const QVariant& val) {
        qreal t = val.toReal();
        setScaleX(t);
        setScaleY(t);
    });
    scaleSeq->addAnimation(anticipate);

    // 冲击
    QPropertyAnimation* impact = new QPropertyAnimation(this, "");
    impact->setDuration(100);
    impact->setStartValue(0.0);
    impact->setEndValue(1.0);
    connect(impact, &QPropertyAnimation::valueChanged, this, [this](const QVariant& val) {
        qreal t = val.toReal();
        qreal baseScale = 0.8 + (1.5 - 0.8) * t;
        qreal squashX = 1.0 + (1.3 - 1.0) * t;
        qreal stretchY = 1.0 - (1.0 - 0.7) * t;
        setScaleX(baseScale * squashX);
        setScaleY(baseScale * stretchY);
    });
    scaleSeq->addAnimation(impact);

    // 回弹
    QPropertyAnimation* bounce = new QPropertyAnimation(this, "");
    bounce->setDuration(100);
    bounce->setStartValue(0.0);
    bounce->setEndValue(1.0);
    connect(bounce, &QPropertyAnimation::valueChanged, this, [this](const QVariant& val) {
        qreal t = val.toReal();
        qreal baseScale = 1.5 - (1.5 - 1.2) * t;
        qreal squashX = 1.3 - (1.3 - 1.0) * t;
        qreal stretchY = 0.7 + (1.0 - 0.7) * t;
        setScaleX(baseScale * squashX);
        setScaleY(baseScale * stretchY);
    });
    scaleSeq->addAnimation(bounce);

    // 消失
    QPropertyAnimation* vanish = new QPropertyAnimation(this, "");
    vanish->setDuration(100);
    vanish->setStartValue(0.0);
    vanish->setEndValue(1.0);
    connect(vanish, &QPropertyAnimation::valueChanged, this, [this](const QVariant& val) {
        qreal t = val.toReal();
        qreal s = 1.2 * (1.0 - t);
        setScaleX(s);
        setScaleY(s);
    });
    scaleSeq->addAnimation(vanish);

    // 闪白
    QPropertyAnimation* flashAnim = new QPropertyAnimation(this, "flash", this);
    flashAnim->setDuration(150);
    flashAnim->setKeyValueAt(0, 0.0);
    flashAnim->setKeyValueAt(0.5, 1.0);
    flashAnim->setKeyValueAt(1, 0.0);
    flashAnim->setEasingCurve(QEasingCurve::OutQuad);

    // 延迟淡出
    QSequentialAnimationGroup* opacitySeq = new QSequentialAnimationGroup(this);
    opacitySeq->addAnimation(new QPauseAnimation(250));
    QPropertyAnimation* opacityAnim = new QPropertyAnimation(this, "opacity", this);
    opacityAnim->setDuration(150);
    opacityAnim->setStartValue(1.0);
    opacityAnim->setEndValue(0.0);
    opacitySeq->addAnimation(opacityAnim);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    group->addAnimation(scaleSeq);
    group->addAnimation(flashAnim);
    group->addAnimation(opacityAnim);

    connect(group, &QParallelAnimationGroup::finished, this, &GameEntity::selectedAnimationFinished);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

qreal GameEntity::scaleX() const { return m_scaleX; }
void GameEntity::setScaleX(qreal scaleX)
{
    if (qFuzzyCompare(m_scaleX, scaleX)) return;
    m_scaleX = scaleX;
    update();
    emit scaleXChanged();
}

qreal GameEntity::scaleY() const { return m_scaleY; }
void GameEntity::setScaleY(qreal scaleY)
{
    if (qFuzzyCompare(m_scaleY, scaleY)) return;
    m_scaleY = scaleY;
    update();
    emit scaleYChanged();
}

qreal GameEntity::flash() const { return m_flash; }
void GameEntity::setFlash(qreal flash)
{
    if (qFuzzyCompare(m_flash, flash)) return;
    m_flash = qBound(0.0, flash, 1.0);
    update();
    emit flashChanged();
}