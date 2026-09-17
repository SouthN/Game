/* ------------------------------------------------------------------
// 文件名     : planegamewindow.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 太空大战游戏主窗口，负责组装 MVC 模块及顶部 UI 状态展示
------------------------------------------------------------------ */
#pragma once
#include <QMainWindow>
#include <QGraphicsView>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "planegamedata.h"
#include "planegameview.h"
#include "planegamecontroller.h"
#include "planegameconfig.h"
#include "planegamesettingsdialog.h"
#include "postprocessglwidget.h"

class PlaneGameWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PlaneGameWindow(QWidget* parent = nullptr);
    ~PlaneGameWindow() override;

    // ========= 禁用拷贝和赋值 =========
    PlaneGameWindow(const PlaneGameWindow&) = delete;
    PlaneGameWindow& operator=(const PlaneGameWindow&) = delete;
    // ========================================

private slots:
    void onPlayPauseClicked(); // 合并后的开始/暂停/恢复逻辑
    void onBagClicked();       // 背包按钮点击逻辑
	void onStatsChanged(int success, int fail); // 监听统计数据变化以更新界面显示
    void onLevelChanged(int level);
    void onGameStateChanged(int state);
    void onSettingsClicked();
    void onExitGame();
	void onScoreChanged(int score); // 【新增】监听分数变化以更新界面显示
	void onPlayerHpChanged(float currentHp, float maxHp); // 【新增】监听玩家生命值变化以更新界面显示
    void onResolutionChanged(int w, int h); // 监听分辨率变化

protected:
    // 【新增】重写鼠标事件以支持无边框拖拽
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void initUI();
    void initConnections();
    void initMVC();
    void updateHexButton(QPushButton* btn, const QString& iconName, const QString& text); // 动态刷新按钮外观

    // MVC组件
    PlaneGameData* m_gameData = nullptr;
    PlaneGameView* m_gameView = nullptr;
    PlaneGameController* m_gameController = nullptr; 

    // UI控件
    QGraphicsView* m_gameViewWidget = nullptr;
    PostProcessGLWidget* glWidget = nullptr;

    // 合并后的核心操作按钮
    QPushButton* m_playPauseBtn = nullptr;

    QPushButton* m_settingsBtn = nullptr;
    QLabel* m_successLabel = nullptr;
    QLabel* m_failLabel = nullptr;
    QLabel* m_accuracyLabel = nullptr;
    QLabel* m_levelLabel = nullptr;

    // 【新增】无边框拖拽相关状态
    bool m_isDragging = false;
    QPoint m_dragPosition;

    // 【新增】自定义标题栏控件
    QWidget* m_titleBar = nullptr;
    QLabel* m_titleLabel = nullptr;
	QPushButton* m_closeBtn = nullptr; // ESC 关闭按钮

    QLabel* m_scoreLabel = nullptr;     // 实时分数标签
    QProgressBar* m_hpBar = nullptr;    // 玩家血条进度条

    // ========== 重构：底部三大面板容器 ==========
    QWidget* m_bottomContainer = nullptr;
    QWidget* m_leftPanel = nullptr;
    QWidget* m_centerPanel = nullptr;
    QWidget* m_rightPanel = nullptr;

    // ========== 扩展：右侧面板六边形控制按钮 ==========
    QPushButton* m_statsBtn = nullptr;   // 分数统计
    QPushButton* m_bagBtn = nullptr;     // 背包
    QPushButton* m_upgradeBtn = nullptr; // 飞机属性
    QPushButton* m_exitBtn = nullptr;    // 退出游戏

};