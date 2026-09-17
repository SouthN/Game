#pragma once
#include <QWidget>
#include <QLabel>
#include "backpackdata.h"

class QEvent;

class BackpackItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BackpackItemWidget(const BackpackItemData& itemData, QWidget* parent = nullptr);
    ~BackpackItemWidget() override = default;

    BackpackItemData getItemData() const { return m_itemData; }
    void setSelected(bool isSelected);
    bool isSelected() const { return m_isSelected; }

signals:
    void itemClicked(BackpackItemWidget* itemWidget);

protected:
	void enterEvent(QEvent* event) override; // 鼠标进入事件：高亮显示
	void leaveEvent(QEvent* event) override; // 鼠标离开事件：取消高亮显示
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void initUI();
    void updateIconPixmap();
    QColor getQualityColor(ItemQuality quality) const;

    BackpackItemData m_itemData;
    bool m_isSelected = false;
	bool m_isHovered = false; // 鼠标悬停状态
    QLabel* m_iconLabel = nullptr;
    QLabel* m_countLabel = nullptr;
};