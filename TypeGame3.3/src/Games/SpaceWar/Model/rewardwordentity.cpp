#include "rewardwordentity.h"
#include "entityfactory.h"
#include <QPainterPath>

RewardWordEntity::RewardWordEntity()
{
    m_rect = QRectF(0, 0, 200, 50); // 预估宽度
    setBoundingRect(m_rect);
    setZValue(100); // 保持在最上层
}

void RewardWordEntity::setWord(const QString& word)
{
    m_word = word;
    m_typedIndex = 0;
    // 根据单词长度动态调整包围盒
    m_rect = QRectF(0, 0, word.length() * 25.0, 50);
    setBoundingRect(m_rect);
}

bool RewardWordEntity::checkNextLetter(QChar c)
{
    if (m_typedIndex < m_word.length() && m_word.at(m_typedIndex) == c) {
        m_typedIndex++;
        return true;
    }
    return false;
}

bool RewardWordEntity::isCompleted() const
{
    return m_typedIndex >= m_word.length();
}

void RewardWordEntity::updateLogic(float deltaTime)
{
    if (!isAlive()) return;

    // 横向向左飞行
    setPos(x() - m_speed * deltaTime, y());

    // 飞出左侧屏幕边缘则销毁
    if (x() + m_rect.width() < 0) {
        setAlive(false);
    }
}

void RewardWordEntity::paint(QPainter* painter)
{
    if (!isAlive()) return;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(m_opacity);

    QFont font("Consolas", 28, QFont::Bold, true);
    painter->setFont(font);

    // 绘制炫酷边框底底板
    painter->setBrush(QColor(10, 20, 40, 180));
    painter->setPen(QPen(QColor(0, 255, 204), 2));
    painter->drawRoundedRect(m_rect, 5, 5);

    float startX = 10.0f;
    float baselineY = 35.0f;

    for (int i = 0; i < m_word.length(); ++i) {
        QString charStr = m_word.at(i);

        // 已输入字符显示为科幻高亮绿，未输入为纯白
        if (i < m_typedIndex) {
            painter->setPen(QColor(0, 255, 100)); // 荧光绿
        }
        else {
            painter->setPen(QColor(255, 255, 255));
        }

        painter->drawText(QPointF(startX, baselineY), charStr);
        startX += QFontMetrics(font).horizontalAdvance(charStr) + 2;
    }
}

void RewardWordEntity::destroy()
{
    EntityFactory::instance()->destroyEntity<RewardWordEntity>(this);
}