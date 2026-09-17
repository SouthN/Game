#include "backpackitemwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QEvent>

BackpackItemWidget::BackpackItemWidget(const BackpackItemData& itemData, QWidget* parent)
    : QWidget(parent), m_itemData(itemData)
{
    setFixedSize(86, 86);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_Hover, true);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    initUI();
}

void BackpackItemWidget::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(0);

    // 物品图标
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(78, 78);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // 物品数量（和参考图一致：右下角半透黑底）
    m_countLabel = new QLabel(QString::number(m_itemData.count), this);
    m_countLabel->setStyleSheet(
        "color: white; font-size: 14px; font-weight: bold;"
        "background-color: rgba(0,0,0,0.55); border-radius: 3px;"
        "padding: 2px 4px; margin: 0px 2px 2px 0px;"
    );
    m_countLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    m_countLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    m_countLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_countLabel->raise();

    mainLayout->addWidget(m_iconLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_countLabel, 0, Qt::AlignRight | Qt::AlignBottom);

    setToolTip(QString("%1\n%2").arg(m_itemData.itemName, m_itemData.description));
    updateIconPixmap();
}

void BackpackItemWidget::updateIconPixmap()
{
    if (!m_iconLabel) return;

    const QSize iconSize = m_isHovered ? QSize(70, 70) : QSize(64, 64);
    QPixmap iconPixmap = BackpackData::loadItemIconPixmap(m_itemData, iconSize);
    if (!iconPixmap.isNull())
    {
        m_iconLabel->setPixmap(iconPixmap);
    }
    else
    {
        m_iconLabel->clear();
    }
}

void BackpackItemWidget::setSelected(bool isSelected)
{
    m_isSelected = isSelected;
    update();
}

QColor BackpackItemWidget::getQualityColor(ItemQuality quality) const
{
    switch (quality)
    {
    case QUALITY_N:         return QColor(168, 172, 178);  // N：灰银
    case QUALITY_R:         return QColor(68, 150, 255);   // R：蓝
    case QUALITY_SR:        return QColor(178, 92, 255);   // SR：紫
    case QUALITY_SSR:       return QColor(255, 205, 68);   // SSR：金
    case QUALITY_SSR_PLUS:  return QColor(255, 116, 46);   // SSR+：橙红
    case QUALITY_UR:        return QColor(108, 246, 255);  // UR：冰蓝幻彩主色，避免和SR紫色混淆
    default:                return QColor(168, 172, 178);
    }
}

void BackpackItemWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRectF rect = this->rect();
    QRectF cellRect = rect.adjusted(3, 3, -3, -3);
    if (m_isHovered)
    {
        cellRect.translate(0, -2);
    }
    const QRectF innerRect = cellRect.adjusted(5, 5, -5, -5);

    QColor qualityColor = getQualityColor(m_itemData.quality);
    const bool isUrQuality = (m_itemData.quality == QUALITY_UR);

    // UR 专用幻彩色组：主色改为冰蓝，并加入金、绿、紫，和 SR 的纯紫边框明显区分
    const QColor urCyan(108, 246, 255);
    const QColor urBlue(86, 142, 255);
    const QColor urMagenta(245, 86, 255);
    const QColor urGold(255, 226, 82);
    const QColor urGreen(94, 255, 170);
    const QColor urWhite(245, 255, 255);

    auto mixColor = [](const QColor& a, const QColor& b, qreal t) -> QColor {
        return QColor(
            static_cast<int>(a.red() * (1.0 - t) + b.red() * t),
            static_cast<int>(a.green() * (1.0 - t) + b.green() * t),
            static_cast<int>(a.blue() * (1.0 - t) + b.blue() * t),
            static_cast<int>(a.alpha() * (1.0 - t) + b.alpha() * t)
        );
        };

    QColor deepBase(3, 13, 28, 245);
    QColor midBase(8, 30, 58, 240);
    QColor topBase(13, 52, 88, 235);

    // 1. 外阴影：悬浮时阴影更深，并略微上浮。
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, m_isHovered ? 170 : 115));
    painter.drawRoundedRect(cellRect.adjusted(1.5, m_isHovered ? 4.0 : 2.5, 1.5, m_isHovered ? 4.0 : 2.5), 8, 8);

    if (isUrQuality)
    {
        // 2-UR. UR 主背景：不是单纯紫色，而是偏冰蓝/金/绿的宝石渐变
        QLinearGradient urBg(cellRect.topLeft(), cellRect.bottomRight());
        urBg.setColorAt(0.00, QColor(14, 72, 92, 245));
        urBg.setColorAt(0.30, QColor(24, 38, 88, 245));
        urBg.setColorAt(0.56, QColor(50, 43, 78, 245));
        urBg.setColorAt(0.78, QColor(32, 74, 58, 245));
        urBg.setColorAt(1.00, QColor(4, 12, 30, 248));
        painter.setBrush(urBg);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(cellRect, 8, 8);

        // 3-UR. 外层幻彩光晕：加粗且多色，解决 UR 边框不明显的问题
        QLinearGradient urOuterGlow(cellRect.topLeft(), cellRect.bottomRight());
        urOuterGlow.setColorAt(0.00, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), m_isSelected ? 150 : (m_isHovered ? 125 : 90)));
        urOuterGlow.setColorAt(0.28, QColor(urWhite.red(), urWhite.green(), urWhite.blue(), m_isSelected ? 180 : (m_isHovered ? 145 : 100)));
        urOuterGlow.setColorAt(0.50, QColor(urGold.red(), urGold.green(), urGold.blue(), m_isSelected ? 160 : (m_isHovered ? 135 : 95)));
        urOuterGlow.setColorAt(0.72, QColor(urGreen.red(), urGreen.green(), urGreen.blue(), m_isSelected ? 145 : (m_isHovered ? 120 : 85)));
        urOuterGlow.setColorAt(1.00, QColor(urMagenta.red(), urMagenta.green(), urMagenta.blue(), m_isSelected ? 140 : (m_isHovered ? 115 : 80)));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QBrush(urOuterGlow), m_isSelected ? 5.4 : (m_isHovered ? 4.1 : 3.0)));
        painter.drawRoundedRect(cellRect.adjusted(-1.0, -1.0, 1.0, 1.0), 10, 10);

        // 4-UR. 主边框：连续彩虹渐变，而不是单色紫边
        QLinearGradient urBorder(cellRect.topLeft(), cellRect.bottomRight());
        urBorder.setColorAt(0.00, urCyan);
        urBorder.setColorAt(0.18, urWhite);
        urBorder.setColorAt(0.38, urGold);
        urBorder.setColorAt(0.58, urGreen);
        urBorder.setColorAt(0.78, urBlue);
        urBorder.setColorAt(1.00, urMagenta);
        painter.setPen(QPen(QBrush(urBorder), m_isSelected ? 2.6 : (m_isHovered ? 2.3 : 1.9)));
        painter.drawRoundedRect(cellRect.adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);

        // 5-UR. 内发光：中心冰蓝，外圈金绿，视觉上和 SR 紫光分离
        QRadialGradient urGlow(cellRect.center(), cellRect.width() * 0.82);
        urGlow.setColorAt(0.00, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), m_isSelected ? 110 : (m_isHovered ? 88 : 62)));
        urGlow.setColorAt(0.42, QColor(urGold.red(), urGold.green(), urGold.blue(), m_isSelected ? 58 : (m_isHovered ? 48 : 34)));
        urGlow.setColorAt(0.70, QColor(urGreen.red(), urGreen.green(), urGreen.blue(), m_isSelected ? 34 : (m_isHovered ? 28 : 20)));
        urGlow.setColorAt(1.00, QColor(urMagenta.red(), urMagenta.green(), urMagenta.blue(), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(urGlow);
        painter.drawRoundedRect(cellRect.adjusted(2, 2, -2, -2), 7, 7);
    }
    else
    {
        // 2. 普通品质背景：深蓝 + 品质染色
        QColor bgTop = mixColor(topBase, qualityColor, 0.22);
        QColor bgMid = mixColor(midBase, qualityColor, 0.16);
        QColor bgBottom = mixColor(deepBase, qualityColor, 0.10);

        QColor borderQuality(
            qualityColor.red(),
            qualityColor.green(),
            qualityColor.blue(),
            m_isSelected ? 240 : (m_isHovered ? 220 : 175)
        );

        QLinearGradient bgGradient(cellRect.topLeft(), cellRect.bottomLeft());
        bgGradient.setColorAt(0.0, bgTop);
        bgGradient.setColorAt(0.45, bgMid);
        bgGradient.setColorAt(1.0, bgBottom);

        painter.setBrush(bgGradient);
        painter.setPen(QPen(borderQuality, m_isSelected ? 1.8 : (m_isHovered ? 1.6 : 1.2)));
        painter.drawRoundedRect(cellRect, 8, 8);

        // 3. 品质内发光
        QRadialGradient qualityGlow(cellRect.center(), cellRect.width() * 0.75);
        qualityGlow.setColorAt(0.0, QColor(
            qualityColor.red(),
            qualityColor.green(),
            qualityColor.blue(),
            m_isSelected ? 75 : (m_isHovered ? 62 : 38)
        ));
        qualityGlow.setColorAt(0.58, QColor(
            qualityColor.red(),
            qualityColor.green(),
            qualityColor.blue(),
            m_isSelected ? 34 : (m_isHovered ? 28 : 18)
        ));
        qualityGlow.setColorAt(1.0, QColor(
            qualityColor.red(),
            qualityColor.green(),
            qualityColor.blue(),
            0
        ));

        painter.setPen(Qt::NoPen);
        painter.setBrush(qualityGlow);
        painter.drawRoundedRect(cellRect.adjusted(2, 2, -2, -2), 7, 7);
    }

    // 4. 顶部金属冷光，UR 使用更亮的棱镜高光
    QRectF topLightRect = cellRect.adjusted(3, 3, -3, -cellRect.height() * 0.62);
    QLinearGradient topLight(topLightRect.topLeft(), topLightRect.bottomRight());
    if (isUrQuality)
    {
        topLight.setColorAt(0.00, QColor(urWhite.red(), urWhite.green(), urWhite.blue(), m_isSelected ? 112 : (m_isHovered ? 98 : 76)));
        topLight.setColorAt(0.48, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), m_isSelected ? 88 : (m_isHovered ? 72 : 52)));
        topLight.setColorAt(1.00, QColor(urGold.red(), urGold.green(), urGold.blue(), 0));
    }
    else
    {
        topLight.setColorAt(0.0, QColor(180, 245, 255, m_isSelected ? 82 : (m_isHovered ? 68 : 48)));
        topLight.setColorAt(1.0, QColor(180, 245, 255, 0));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(topLight);
    painter.drawRoundedRect(topLightRect, 6, 6);

    // 5. 内框
    painter.setBrush(Qt::NoBrush);
    if (isUrQuality)
    {
        QLinearGradient urInner(innerRect.topLeft(), innerRect.bottomRight());
        urInner.setColorAt(0.00, QColor(urWhite.red(), urWhite.green(), urWhite.blue(), m_isSelected ? 230 : 150));
        urInner.setColorAt(0.32, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), m_isSelected ? 210 : 135));
        urInner.setColorAt(0.66, QColor(urGold.red(), urGold.green(), urGold.blue(), m_isSelected ? 190 : 115));
        urInner.setColorAt(1.00, QColor(urMagenta.red(), urMagenta.green(), urMagenta.blue(), m_isSelected ? 185 : 110));
        painter.setPen(QPen(QBrush(urInner), 1.1));
    }
    else
    {
        painter.setPen(QPen(QColor(
            qualityColor.red(),
            qualityColor.green(),
            qualityColor.blue(),
            m_isSelected ? 155 : 85
        ), 1.0));
    }
    painter.drawRoundedRect(innerRect, 5, 5);

    // 6. 底部品质能量条；UR 使用高对比棱镜条
    QLinearGradient qualityBar(cellRect.bottomLeft(), cellRect.bottomRight());
    if (isUrQuality)
    {
        qualityBar.setColorAt(0.00, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), 55));
        qualityBar.setColorAt(0.16, QColor(urWhite.red(), urWhite.green(), urWhite.blue(), 245));
        qualityBar.setColorAt(0.34, QColor(urGold.red(), urGold.green(), urGold.blue(), 255));
        qualityBar.setColorAt(0.52, QColor(urGreen.red(), urGreen.green(), urGreen.blue(), 245));
        qualityBar.setColorAt(0.72, QColor(urBlue.red(), urBlue.green(), urBlue.blue(), 235));
        qualityBar.setColorAt(0.90, QColor(urMagenta.red(), urMagenta.green(), urMagenta.blue(), 245));
        qualityBar.setColorAt(1.00, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), 55));
    }
    else
    {
        qualityBar.setColorAt(0.0, QColor(qualityColor.red(), qualityColor.green(), qualityColor.blue(), 20));
        qualityBar.setColorAt(0.5, QColor(qualityColor.red(), qualityColor.green(), qualityColor.blue(), 220));
        qualityBar.setColorAt(1.0, QColor(qualityColor.red(), qualityColor.green(), qualityColor.blue(), 20));
    }

    painter.setPen(QPen(QBrush(qualityBar), m_isSelected ? 3.3 : 2.4));
    painter.drawLine(
        QPointF(cellRect.left() + 10, cellRect.bottom() - 4),
        QPointF(cellRect.right() - 10, cellRect.bottom() - 4)
    );

    // 7. 科技四角折线，UR 使用渐变折线
    if (isUrQuality)
    {
        QLinearGradient urCorner(cellRect.topLeft(), cellRect.bottomRight());
        urCorner.setColorAt(0.00, urCyan);
        urCorner.setColorAt(0.33, urGold);
        urCorner.setColorAt(0.66, urGreen);
        urCorner.setColorAt(1.00, urMagenta);
        painter.setPen(QPen(QBrush(urCorner), m_isSelected ? 2.0 : 1.45));
    }
    else
    {
        painter.setPen(QPen(QColor(
            qualityColor.red(),
            qualityColor.green(),
            qualityColor.blue(),
            m_isSelected ? 235 : 155
        ), m_isSelected ? 1.7 : 1.2));
    }
    painter.setBrush(Qt::NoBrush);

    const qreal l = cellRect.left();
    const qreal r = cellRect.right();
    const qreal t = cellRect.top();
    const qreal b = cellRect.bottom();
    const qreal corner = 13;

    painter.drawLine(QPointF(l + 5, t + corner), QPointF(l + 5, t + 6));
    painter.drawLine(QPointF(l + 5, t + 6), QPointF(l + corner, t + 6));

    painter.drawLine(QPointF(r - corner, t + 6), QPointF(r - 5, t + 6));
    painter.drawLine(QPointF(r - 5, t + 6), QPointF(r - 5, t + corner));

    painter.drawLine(QPointF(l + 5, b - corner), QPointF(l + 5, b - 6));
    painter.drawLine(QPointF(l + 5, b - 6), QPointF(l + corner, b - 6));

    painter.drawLine(QPointF(r - corner, b - 6), QPointF(r - 5, b - 6));
    painter.drawLine(QPointF(r - 5, b - 6), QPointF(r - 5, b - corner));

    if (isUrQuality)
    {
        // UR 专属斜向棱镜高光，增强“最高品质”的可识别度
        QLinearGradient prismLine(cellRect.topLeft(), cellRect.bottomRight());
        prismLine.setColorAt(0.00, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), 0));
        prismLine.setColorAt(0.48, QColor(urWhite.red(), urWhite.green(), urWhite.blue(), m_isSelected ? 175 : 105));
        prismLine.setColorAt(0.55, QColor(urGold.red(), urGold.green(), urGold.blue(), m_isSelected ? 130 : 76));
        prismLine.setColorAt(1.00, QColor(urMagenta.red(), urMagenta.green(), urMagenta.blue(), 0));
        painter.setPen(QPen(QBrush(prismLine), m_isSelected ? 2.2 : 1.4));
        painter.drawLine(QPointF(cellRect.left() + 12, cellRect.top() + 8),
            QPointF(cellRect.right() - 8, cellRect.bottom() - 13));
    }

    // 8. 悬浮态：鼠标移入时增加一层柔和高光，强化“浮在格子上”的反馈。
    if (m_isHovered)
    {
        QLinearGradient hoverLight(cellRect.topLeft(), cellRect.bottomRight());
        hoverLight.setColorAt(0.00, QColor(255, 255, 255, isUrQuality ? 34 : 26));
        hoverLight.setColorAt(0.42, QColor(255, 255, 255, 10));
        hoverLight.setColorAt(1.00, QColor(255, 255, 255, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(hoverLight);
        painter.drawRoundedRect(cellRect.adjusted(1, 1, -1, -1), 7, 7);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 255, 255, m_isSelected ? 135 : 105), 1.2));
        painter.drawRoundedRect(cellRect.adjusted(1.2, 1.2, -1.2, -1.2), 7, 7);
    }

    // 9. 选中态：UR 选中时使用幻彩双层边框，其他品质仍使用单色高亮
    if (m_isSelected)
    {
        if (isUrQuality)
        {
            QLinearGradient urSelected(cellRect.topLeft(), cellRect.bottomRight());
            urSelected.setColorAt(0.00, urCyan);
            urSelected.setColorAt(0.18, urWhite);
            urSelected.setColorAt(0.38, urGold);
            urSelected.setColorAt(0.58, urGreen);
            urSelected.setColorAt(0.78, urBlue);
            urSelected.setColorAt(1.00, urMagenta);

            painter.setPen(QPen(QBrush(urSelected), 6.2));
            painter.drawRoundedRect(cellRect.adjusted(-1.8, -1.8, 1.8, 1.8), 11, 11);

            painter.setPen(QPen(QBrush(urSelected), 2.8));
            painter.drawRoundedRect(cellRect.adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);

            painter.setPen(QPen(QColor(255, 255, 255, 220), 1.1));
            painter.drawRoundedRect(innerRect.adjusted(-1, -1, 1, 1), 6, 6);

            QLinearGradient selectedLine(cellRect.topLeft(), cellRect.topRight());
            selectedLine.setColorAt(0.00, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), 0));
            selectedLine.setColorAt(0.18, QColor(urWhite.red(), urWhite.green(), urWhite.blue(), 255));
            selectedLine.setColorAt(0.38, QColor(urGold.red(), urGold.green(), urGold.blue(), 255));
            selectedLine.setColorAt(0.62, QColor(urGreen.red(), urGreen.green(), urGreen.blue(), 255));
            selectedLine.setColorAt(0.82, QColor(urMagenta.red(), urMagenta.green(), urMagenta.blue(), 255));
            selectedLine.setColorAt(1.00, QColor(urCyan.red(), urCyan.green(), urCyan.blue(), 0));

            painter.setPen(QPen(QBrush(selectedLine), 2.6));
            painter.drawLine(
                QPointF(cellRect.left() + 10, cellRect.top() + 4),
                QPointF(cellRect.right() - 10, cellRect.top() + 4)
            );
        }
        else
        {
            // 外圈光晕
            painter.setPen(QPen(QColor(
                qualityColor.red(),
                qualityColor.green(),
                qualityColor.blue(),
                110
            ), 5.0));
            painter.drawRoundedRect(cellRect.adjusted(-1.5, -1.5, 1.5, 1.5), 10, 10);

            // 主高亮边框
            painter.setPen(QPen(QColor(
                qualityColor.red(),
                qualityColor.green(),
                qualityColor.blue(),
                255
            ), 2.5));
            painter.drawRoundedRect(cellRect.adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);

            // 内部亮线
            painter.setPen(QPen(QColor(
                qualityColor.lighter(155).red(),
                qualityColor.lighter(155).green(),
                qualityColor.lighter(155).blue(),
                235
            ), 1.2));
            painter.drawRoundedRect(innerRect.adjusted(-1, -1, 1, 1), 6, 6);

            // 顶部选中流光
            QLinearGradient selectedLine(cellRect.topLeft(), cellRect.topRight());
            selectedLine.setColorAt(0.0, QColor(qualityColor.red(), qualityColor.green(), qualityColor.blue(), 0));
            selectedLine.setColorAt(0.5, QColor(qualityColor.lighter(170).red(), qualityColor.lighter(170).green(), qualityColor.lighter(170).blue(), 255));
            selectedLine.setColorAt(1.0, QColor(qualityColor.red(), qualityColor.green(), qualityColor.blue(), 0));

            painter.setPen(QPen(QBrush(selectedLine), 2.2));
            painter.drawLine(
                QPointF(cellRect.left() + 12, cellRect.top() + 4),
                QPointF(cellRect.right() - 12, cellRect.top() + 4)
            );
        }
    }
}


void BackpackItemWidget::enterEvent(QEvent* event)
{
    m_isHovered = true;
    raise();
    updateIconPixmap();
    update();
    QWidget::enterEvent(event);
}

void BackpackItemWidget::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    updateIconPixmap();
    update();
    QWidget::leaveEvent(event);
}

void BackpackItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) emit itemClicked(this);
    QWidget::mousePressEvent(event);
}