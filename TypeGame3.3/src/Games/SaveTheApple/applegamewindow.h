/* ------------------------------------------------------------------
// 文件名     : applegamewindow.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果游戏主窗口，负责组装 MVC 模块及顶部 UI 状态展示
------------------------------------------------------------------ */
#pragma once
#include <QMainWindow>
#include <QGraphicsView>
#include <QPushButton>
#include <QLabel>
#include "applegamedata.h"
#include "applegameview.h"
#include "applegamecontroller.h"
#include "applegameconfig.h"
#include "applegamesettingsdialog.h"
#include "postprocessglwidget.h" // 新增：引入自定义控件

class AppleGameWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit AppleGameWindow(QWidget* parent = nullptr);
    ~AppleGameWindow() override;

    // ========= 新增：禁用拷贝和赋值 =========
    AppleGameWindow(const AppleGameWindow&) = delete;
    AppleGameWindow& operator=(const AppleGameWindow&) = delete;
    // ========================================

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onStatsChanged(int success, int fail);
    void onLevelChanged(int level);
    void onGameStateChanged(int state);
    void onSettingsClicked();
    void onExitGame(); // 新增：退出游戏槽
    void onResolutionChanged(int w, int h); // 监听分辨率变化

private:
    void initUI();
    void initConnections();
    void initMVC();

    // MVC组件
    AppleGameData* m_gameData = nullptr;
    AppleGameView* m_gameView = nullptr;
    AppleGameController* m_gameController = nullptr;

    // UI控件
    QGraphicsView* m_gameViewWidget = nullptr;
    PostProcessGLWidget* glWidget = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_pauseBtn = nullptr;
    QPushButton* m_settingsBtn = nullptr;
    QLabel* m_successLabel = nullptr;
    QLabel* m_failLabel = nullptr;
    QLabel* m_accuracyLabel = nullptr;
    QLabel* m_levelLabel = nullptr;

};