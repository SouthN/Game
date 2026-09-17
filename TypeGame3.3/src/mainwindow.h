/* ------------------------------------------------------------------
// 文件名     : mainwindow.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2026-XX-XX
// 功能描述   : 游戏启动器主窗口逻辑，负责子游戏入口分发与界面交互
------------------------------------------------------------------ */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QMouseEvent>
#include <QList>

// 游戏信息结构体（完全保留原有映射关系）
struct GameInfo {
    QString name;
    QString iconPath;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // ========= 新增：禁用拷贝和赋值 =========
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    // ========================================

protected:
    // 窗口拖动事件（完全保留原有逻辑）
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onAppleGameClicked();
    void onSpaceWarClicked();
    void onGameCardClicked();

private:
    void initUI();
    void initTopBar();
    void initNavBar();
    void initGameArea();
    void initBottomBar();

    // UI模块控件
    QWidget* m_topBar = nullptr;
    QWidget* m_navBar = nullptr;
    QWidget* m_gameArea = nullptr;
    QWidget* m_bottomBar = nullptr;

    // 窗口拖动变量
    bool m_isDragging = false;
    QPoint m_dragStartPos;

    QList<QWidget*> m_gameCardList;
    QList<GameInfo> gameInfos;
};

#endif // MAINWINDOW_H