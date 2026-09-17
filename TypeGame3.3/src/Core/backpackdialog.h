#pragma once
#include <QDialog>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QGridLayout>
#include <QScrollArea>
#include "backpackdata.h"
#include "backpackitemwidget.h"

class BackpackDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BackpackDialog(QWidget* parent = nullptr);
    ~BackpackDialog() override = default;

    BackpackDialog(const BackpackDialog&) = delete;
    BackpackDialog& operator=(const BackpackDialog&) = delete;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onCategoryChanged(int categoryId); // 分类切换，刷新物品显示
    void onItemClicked(BackpackItemWidget* itemWidget); // 物品被点击，显示详情
    void onCloseClicked(); // 关闭背包界面
    void refreshBackpackData(); // 刷新背包物品显示
    void refreshResourceInfo(); // 刷新钻石、金币和容量信息

    // 新增：使用按钮点击槽函数
    void onUseClicked();

private:
    // 【核心工具函数】递归设置所有子控件背景透明，彻底解决白色背景
    void setAllWidgetTransparent(QWidget* widget);
    void initUI();
    void initTopBar();
    void initCategoryBar();
    void initItemGrid(); // 初始化物品网格
    void initDetailPanel();

    // 顶部栏控件
    QWidget* m_topBarWidget = nullptr;
    QLabel* m_diamondLabel = nullptr;
    QLabel* m_goldLabel = nullptr;
    QLabel* m_capacityLabel = nullptr;
    QPushButton* m_closeBtn = nullptr;

    // 分类栏
    QList<QToolButton*> m_categoryBtnList;
    int m_currentCategoryId = -1; // -1 表示“所有”分类

    // 物品网格
    QGridLayout* m_itemGridLayout = nullptr;
    QScrollArea* m_itemScrollArea = nullptr;
    QList<BackpackItemWidget*> m_itemWidgetList;
    BackpackItemWidget* m_currentSelectedItem = nullptr;

    // 详情面板
    QWidget* m_detailPanelWidget = nullptr;
    QLabel* m_itemNameLabel = nullptr;
    QLabel* m_itemCategoryLabel = nullptr;
    QLabel* m_itemIconDetailLabel = nullptr;
    QLabel* m_itemDescLabel = nullptr;
    // 新增：使用按钮
    QPushButton* m_useBtn = nullptr;

    // 窗口拖动
    bool m_isDragging = false;
    QPoint m_dragStartPos;
};