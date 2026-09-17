#include "applegamewindow.h"
#include "commondefs.h"
#include "audiomanager.h" // 新增头文件

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>

#include <QFile>
#include <QTextStream>
#include <QOpenGLWidget>

AppleGameWindow::AppleGameWindow(QWidget* parent)
    : QMainWindow(parent)
{
    initMVC();
    initUI();
    initConnections();

    // 【修改】：窗口高度 = 游戏视口高度 + 底部控制栏高度(100)
    QSize res = AppleGameConfig::instance()->getResolution();
    resize(res.width(), res.height() + 100);

    setWindowTitle(tr("game_save_the_apple"));
    // 【修改】：游戏视口完全对齐设定的分辨率，不扣减！
    m_gameView->initScene(res.width(), res.height());

    if (glWidget) {
        glWidget->updateGeometry();
    }

    // ========== 新增：加载游戏样式，仅作用于当前窗口 ==========
    QFile styleFile(":/SaveTheApple/assets/qss/AppleGame_OrchardStyle.qss"); // 加载果园森系游戏样式
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        stream.setCodec("UTF-8");
        QString styleSheet = stream.readAll();
        // 仅给当前游戏窗口设置样式，不影响全局
        this->setStyleSheet(styleSheet);
        styleFile.close();
    }
    // =====================================================================
}

AppleGameWindow::~AppleGameWindow()
{
    // ========== 【核心修复 1：防野指针崩溃】 ==========
    // 在销毁数据和场景前，必须先断开与 UI 视口的绑定，并隐藏控件
    // 防止在析构期间触发 PaintEvent 导致渲染线程访问已被销毁的 m_gameView
    if (glWidget) {
        glWidget->setBindGraphicsView(nullptr); 
        glWidget->hide();
    }
    if (m_gameViewWidget) {
        m_gameViewWidget->setScene(nullptr);
    }
    // ===================================================
    // 
    // 【新增】先停止游戏，清理场景资源
    if (m_gameController) {
        m_gameController->stopGame();
    }
    if (m_gameView) {
        m_gameView->clearScene(); // 提前清理背景资源，此时上下文有效
    }
    SAFE_DELETE(m_gameController);
    SAFE_DELETE(m_gameView);
    SAFE_DELETE(m_gameData);
}

void AppleGameWindow::initMVC()
{
	AppleGameConfig* config = AppleGameConfig::instance(); // 获取配置单例，确保数据模型和控制器使用同一配置对象，保持数据一致性
    m_gameData = new AppleGameData(config);
    m_gameView = new AppleGameView(m_gameData);
    m_gameController = new AppleGameController(m_gameData, m_gameView);
}

void AppleGameWindow::initUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 游戏视图
    m_gameViewWidget = new QGraphicsView(m_gameView, this);
    m_gameViewWidget->setRenderHint(QPainter::Antialiasing);
    m_gameViewWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameViewWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameViewWidget->setFocusPolicy(Qt::StrongFocus);
    m_gameViewWidget->hide();

    // ========== 【修改】后处理控件作为最终显示输出 ==========
    glWidget = new PostProcessGLWidget(this);
    glWidget->setBindGraphicsView(m_gameViewWidget); // 绑定游戏场景的 QGraphicsView
    // 【核心新增】给游戏视图绑定OpenGL控件，用于背景资源管理
    m_gameView->setGLWidget(glWidget);
    // ========== 【核心重构：依赖注入】 ==========
    // 将 SaveTheApple 的专属配置单例，作为基类指针注入给 Core 层的通用渲染组件
    glWidget->setConfig(AppleGameConfig::instance());

    mainLayout->addWidget(glWidget, 0, Qt::AlignCenter);

    // ========== 修改：给控制栏包裹容器，设置objectName ==========
    QWidget* controlWidget = new QWidget(this);
    controlWidget->setObjectName("ControlBar");
    QHBoxLayout* controlLayout = new QHBoxLayout(controlWidget);
    // ========== 以下原有代码完全不变 ==========
    controlLayout->setContentsMargins(20, 10, 20, 10);
    controlLayout->setSpacing(15);

    // 控制按钮
    m_startBtn = new QPushButton(tr("start_game"), this);
    m_pauseBtn = new QPushButton(tr("pause_game"), this);
    m_pauseBtn->setEnabled(false);
    m_settingsBtn = new QPushButton(tr("settings"), this);
    controlLayout->addWidget(m_startBtn);
    controlLayout->addWidget(m_pauseBtn);

    // 统计标签
    m_successLabel = new QLabel(tr("stats_success").arg(0), this);
    m_failLabel = new QLabel(tr("stats_fail").arg(0), this);
    m_accuracyLabel = new QLabel(tr("stats_accuracy").arg(0), this);
    m_levelLabel = new QLabel(tr("stats_level").arg(1), this);

    // ========== 新增：给统计标签设置objectName，匹配专属样式 ==========
    m_successLabel->setObjectName("StatsLabel");
    m_failLabel->setObjectName("StatsLabel");
    m_accuracyLabel->setObjectName("StatsLabel");
    m_levelLabel->setObjectName("StatsLabel");
    // ==============================================================

    controlLayout->addWidget(m_successLabel);
    controlLayout->addWidget(m_failLabel);
    controlLayout->addWidget(m_accuracyLabel);
    controlLayout->addWidget(m_levelLabel);
    controlLayout->addStretch();

    // 配置控件（原有代码完全不变）
    controlLayout->addWidget(m_settingsBtn);

    // ========== 修改：把控制栏容器添加到主布局 ==========
    mainLayout->addWidget(controlWidget);
}

void AppleGameWindow::initConnections()
{
	AppleGameConfig* config = AppleGameConfig::instance(); // 获取配置单例

    // 按钮事件
    connect(m_startBtn, &QPushButton::clicked, this, &AppleGameWindow::onStartClicked);
    connect(m_pauseBtn, &QPushButton::clicked, this, &AppleGameWindow::onPauseClicked);
    connect(m_settingsBtn, &QPushButton::clicked, this, &AppleGameWindow::onSettingsClicked);

    // 配置数据更新
    connect(config, &AppleGameConfig::statsChanged, this, &AppleGameWindow::onStatsChanged);
    connect(config, &AppleGameConfig::levelChanged, this, &AppleGameWindow::onLevelChanged);
    // 新增：监听分辨率变更
    connect(config, &AppleGameConfig::resolutionChanged, this, &AppleGameWindow::onResolutionChanged);

    // 游戏状态联动
    connect(m_gameData, &AppleGameData::stateChanged, this, &AppleGameWindow::onGameStateChanged);
    connect(m_gameData, &GameDataBase::dataChanged, m_gameView, &AppleGameView::onDataChanged);

    // 新增：连接退出游戏信号
    connect(m_gameController, &AppleGameController::exitGame, this, &AppleGameWindow::onExitGame);
}

void AppleGameWindow::onStartClicked()
{
    // 新增：播放按钮点击音效+游戏背景音乐
    AudioManager::instance()->playBackgroundMusic(AUDIO_BG_APPLE);

    m_gameController->startGame();
    // 切换按钮状态
    m_startBtn->setEnabled(false);
    m_pauseBtn->setEnabled(true);
    // 视图获取焦点
    glWidget->setFocus();
}

void AppleGameWindow::onPauseClicked()
{
    if (m_gameData->getGameState() == GameDataBase::Playing) {
        m_gameController->pauseGame();
        m_pauseBtn->setText(tr("resume_game"));
        AudioManager::instance()->pauseBackgroundMusic(); // 新增：暂停背景音乐
    } else if (m_gameData->getGameState() == GameDataBase::Paused) {
        m_gameController->resumeGame();
        m_pauseBtn->setText(tr("pause_game"));
        AudioManager::instance()->resumeBackgroundMusic(); // 新增：恢复背景音乐
        glWidget->setFocus();
    }
}

void AppleGameWindow::onStatsChanged(int success, int fail)
{
    m_successLabel->setText(tr("stats_success").arg(success));
    m_failLabel->setText(tr("stats_fail").arg(fail));
    int total = success + fail;
    int accuracy = total > 0 ? (success * 100 / total) : 0;
    m_accuracyLabel->setText(tr("stats_accuracy").arg(accuracy));
}

void AppleGameWindow::onLevelChanged(int level)
{
    m_levelLabel->setText(tr("stats_level").arg(level));
}

// 游戏结束状态处理
void AppleGameWindow::onGameStateChanged(int state)
{
    if (state == GameDataBase::GameOver) {
        AudioManager::instance()->stopBackgroundMusic(); // 新增：游戏结束停止BGM
        QMessageBox::information(this, tr("game_over_title"), tr("game_over_all_levels_passed"));
        // 重置UI状态
        m_startBtn->setEnabled(true);
        m_pauseBtn->setEnabled(false);
        m_pauseBtn->setText(tr("pause_game"));
    }
}

void AppleGameWindow::onSettingsClicked()
{
    // 记录打开设置前的游戏运行状态
    bool wasPlaying = (m_gameData->getGameState() == GameDataBase::Playing);

    // 【核心修改】如果游戏正在运行，自动暂停游戏（复用现有暂停逻辑，同步更新UI）
    if (wasPlaying) {
        onPauseClicked();
    }

    // 创建设置对话框（模态阻塞，用户必须关闭后才能操作游戏窗口）
    AppleGameSettingsDialog settingsDialog(this);
    int dialogResult = settingsDialog.exec();

    // 【核心修改】如果打开前游戏正在运行，关闭对话框后自动恢复游戏
    if (wasPlaying) {
        onPauseClicked();
    }
}

void AppleGameWindow::onExitGame()
{
    AudioManager::instance()->stopBackgroundMusic(); // 新增：停止背景音乐
    close();
}

// 新增槽函数实现
void AppleGameWindow::onResolutionChanged(int w, int h)
{
    // 【修改】：外框包裹附加的UI高度
    this->resize(w, h + 100);
    // 【修改】：核心游戏视图精确对齐选项里的分辨率
    m_gameView->updateResolution(w, h);

    if (glWidget) {
        glWidget->updateGeometry();
    }
}