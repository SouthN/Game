#include "planegamewindow.h"
#include "commondefs.h"
#include "audiomanager.h" // 新增头文件
#include "backpackdialog.h" // 背包界面

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QGridLayout>  // 新增：网格布局
#include <QSpacerItem> // 新增：间距控制
#include <QFormLayout> // 新增：表单布局
#include <QFile>
#include <QTextStream>
#include <QOpenGLWidget>

PlaneGameWindow::PlaneGameWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // 【新增】设置无边框窗口属性
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    initMVC();
    initUI();
    initConnections();

    QSize res = PlaneGameConfig::instance()->getResolution();

    // 【修改】：窗口高度 = 游戏视口高度 + 顶部栏(40) + 底部控制台(160)
    resize(res.width(), res.height() + 200);
    setWindowTitle(tr("game_space_war"));
    
    // 【修改】：游戏视口完全对齐设定的分辨率，这一步内部会调用 initPlayer() 创建玩家战机
    // 【核心改动】：先初始化场景，确保 PlayerEntity 及其组件被创建
    m_gameView->initScene(res.width(), res.height());

    // =====================================================================
    // 【新增】：手动连接玩家属性组件的信号。
    // 因为玩家实体是在 initScene 中创建的，所以连接必须在此之后进行
    // =====================================================================
    if (m_gameView->getPlayer() && m_gameView->getPlayer()->attributes()) {
        connect(m_gameView->getPlayer()->attributes(), &PlayerAttributeComponent::hpChanged,
            this, &PlaneGameWindow::onPlayerHpChanged);
        // 初始同步一次 UI
        onPlayerHpChanged(m_gameView->getPlayer()->getHp(), m_gameView->getPlayer()->getMaxHp());
    }
    // ==============================================================================

    if (glWidget) {
		glWidget->updateGeometry();// 确保 GLWidget 大小正确
    }

    // ========== 加载游戏样式，仅作用于当前窗口 ==========
    // 注意：这里路径替换为太空大战的专属样式表，如果暂时没有，可以先保留或者注释
    QFile styleFile(":/SpaceWar/assets/qss/SpaceWar_NeonStyle.qss"); 
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        stream.setCodec("UTF-8");
        QString styleSheet = stream.readAll();
        this->setStyleSheet(styleSheet);
        styleFile.close();
    }
    // =====================================================================
}

PlaneGameWindow::~PlaneGameWindow()
{
    // ========== 【核心防野指针崩溃】 ==========
    if (glWidget) {
        glWidget->setBindGraphicsView(nullptr);
        glWidget->hide();
    }
    if (m_gameViewWidget) {
        m_gameViewWidget->setScene(nullptr);
    }

    // 先停止游戏，清理场景资源
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

void PlaneGameWindow::initMVC()
{
    PlaneGameConfig* config = PlaneGameConfig::instance();
    m_gameData = new PlaneGameData(config);
    m_gameView = new PlaneGameView(m_gameData);
    m_gameController = new PlaneGameController(m_gameData, m_gameView);
}

void PlaneGameWindow::updateHexButton(QPushButton* btn, const QString& iconName, const QString& text)
{
    if (!btn) return;

    // 更新按钮本身的图标与样式
    QString qss = QString(
        "QPushButton {"
        "   border-image: url(:/MainWindow/assets/images/MainWindow/btn_hex_normal.png);"
        "   image: url(:/MainWindow/assets/images/MainWindow/%1);"
        "   padding: 14px;"
        "   background: transparent;"
        "}"
        "QPushButton:hover {"
        "   border-image: url(:/MainWindow/assets/images/MainWindow/btn_hex_hover.png);"
        "}"
        "QPushButton:pressed {"
        "   padding: 16px;"
        "}"
    ).arg(iconName);
    btn->setStyleSheet(qss);

    // 遍历父容器（复合控件），找到对应的 QLabel 并更新文字
    if (btn->parentWidget()) {
        for (QObject* child : btn->parentWidget()->children()) {
            QLabel* label = qobject_cast<QLabel*>(child);
            if (label) {
                label->setText(text);
                break;
            }
        }
    }
}

void PlaneGameWindow::initUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 回归垂直布局，将标题、游戏、操作台严格上下分离
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ================= 【1. 顶部：自定义标题栏】 =================
    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(40);
    m_titleBar->setStyleSheet("background-color: rgba(20, 20, 30, 255);");
    QHBoxLayout* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);

    m_titleLabel = new QLabel(tr("game_space_war"), this);
    m_titleLabel->setStyleSheet("color: white; font-weight: bold; font-size: 16px;");

    m_closeBtn = new QPushButton("Esc", this);
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setStyleSheet("QPushButton { background: transparent; color: white; font-weight: bold; font-size: 18px; border-radius: 15px; }"
        "QPushButton:hover { background: #E81123; }");

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(m_titleBar);

    // ================= 【2. 中间：游戏主视图】 =================
    m_gameViewWidget = new QGraphicsView(m_gameView, this);
    m_gameViewWidget->setRenderHint(QPainter::Antialiasing);
    m_gameViewWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameViewWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameViewWidget->setFocusPolicy(Qt::StrongFocus);
    m_gameViewWidget->hide();

    glWidget = new PostProcessGLWidget(this);
    glWidget->setBindGraphicsView(m_gameViewWidget);
    m_gameView->setGLWidget(glWidget);
    glWidget->setConfig(PlaneGameConfig::instance());

    // 添加游戏视图，伸展因子设为 1，它会自动占满 650 的高度
    mainLayout->addWidget(glWidget, 1);

    // ================= 【3. 底部：科幻底座面板 (磨砂背景)】 =================
    m_bottomContainer = new QWidget(this);
    m_bottomContainer->setObjectName("BottomContainer");
    m_bottomContainer->setFixedHeight(160);

    // 【核心设计】：深色半透明背景 + 顶部科幻蓝高光描边，模拟控制台玻璃质感
    m_bottomContainer->setStyleSheet(
        "QWidget#BottomContainer { "
        "background-color: rgba(12, 18, 28, 240); "
        "border-top: 1px solid rgba(0, 191, 255, 120); "
        "}"
    );

    QHBoxLayout* bottomLayout = new QHBoxLayout(m_bottomContainer);
    bottomLayout->setContentsMargins(15, 10, 15, 15);
    bottomLayout->setSpacing(15);

    // --- 3.1 左侧面板 ---
    m_leftPanel = new QWidget(this);
    m_leftPanel->setObjectName("LeftPanel");
    m_leftPanel->setStyleSheet("QWidget#LeftPanel { border-image: url(:/MainWindow/assets/images/MainWindow/panel_left.png) 15 15 15 15; background: transparent; }");

    // 左侧面板整体水平布局
    QHBoxLayout* leftLayout = new QHBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(20, 15, 20, 15);
    leftLayout->setSpacing(20);

    // 1. 头像区 (Avatar) - 加一个微小的科幻边框
    QLabel* avatarLabel = new QLabel(m_leftPanel);
    avatarLabel->setFixedSize(110, 110);
    avatarLabel->setStyleSheet(
        "image: url(:/MainWindow/assets/images/MainWindow/avatar_girl.png);"
        "border: 2px solid rgba(0, 191, 255, 120);"
        "border-radius: 4px;"
        "background: rgba(0, 20, 40, 200);"
    );

    // 2. 状态条区 (Bars) 垂直布局
    QWidget* barsWidget = new QWidget(m_leftPanel);
    QVBoxLayout* barsLayout = new QVBoxLayout(barsWidget);
    barsLayout->setContentsMargins(0, 5, 0, 5);
    barsLayout->setSpacing(20); // 增大上下间距，使其在垂直方向上与头像等高对齐

    // -- 2.1 HP 血条行 --
    QWidget* hpRow = new QWidget(barsWidget);
    QHBoxLayout* hpLayout = new QHBoxLayout(hpRow);
    hpLayout->setContentsMargins(0, 0, 0, 0);
    hpLayout->setSpacing(10);

    QLabel* hpIcon = new QLabel(hpRow);
    hpIcon->setFixedSize(30, 30);
    hpIcon->setStyleSheet("border-image: url(:/MainWindow/assets/images/MainWindow/icon_hp.png); background: transparent;");

    QLabel* hpText = new QLabel(tr("HP"), hpRow); // 遵循铁律：杜绝硬编码中文
    hpText->setFixedWidth(30);
    hpText->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: bold; background: transparent;");

    m_hpBar = new QProgressBar(hpRow);
    m_hpBar->setFixedSize(180, 28);
    m_hpBar->setTextVisible(false); // 隐藏原生百分比

    // 【核心黑魔法】：使用 width 和 margin 配合 border-image 平铺，完美还原切块血条！
    m_hpBar->setStyleSheet(
        "QProgressBar { background-color: rgba(0, 20, 40, 150); border: 3px solid rgba(0, 191, 255, 80); border-radius: 2px; }"
        "QProgressBar::chunk { border-image: url(:/MainWindow/assets/images/MainWindow/bar_hp_chunk.png); width: 13px; margin: -1px 0px; }"
    );
    // 【修改】：使用占位初始值。真实的精准浮点血量会在构造函数的 onPlayerHpChanged 被立即同步并覆盖
    int initialHp = 18;
    m_hpBar->setRange(0, initialHp);
    m_hpBar->setValue(initialHp);

    QLabel* hpValueText = new QLabel(QString("%1 / %2").arg(initialHp).arg(initialHp), hpRow);
    hpValueText->setObjectName("HpValueText"); // 设定 ObjectName，方便后续动态查修改
    hpValueText->setStyleSheet("color: #FFFFFF; font-size: 13px; font-family: Consolas; background: transparent;");

    hpLayout->addWidget(hpIcon);
    hpLayout->addWidget(hpText);
    hpLayout->addWidget(m_hpBar);
    hpLayout->addWidget(hpValueText);

    // -- 2.2 Energy 蓄力条 --
    QWidget* energyRow = new QWidget(barsWidget);
    QHBoxLayout* energyLayout = new QHBoxLayout(energyRow);
    energyLayout->setContentsMargins(0, 0, 0, 0);
    energyLayout->setSpacing(10);

    QLabel* energyIcon = new QLabel(energyRow);
    energyIcon->setFixedSize(30, 40);
    energyIcon->setStyleSheet("border-image: url(:/MainWindow/assets/images/MainWindow/icon_energy.png); background: transparent;");

    QLabel* energyText = new QLabel(tr("SP"), energyRow); // 遵循铁律：杜绝硬编码中文
    energyText->setFixedWidth(30);
    energyText->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: bold; background: transparent;");

    QProgressBar* energyBar = new QProgressBar(energyRow);
    energyBar->setFixedSize(180, 28); // 尺寸与血条完全一致
    energyBar->setTextVisible(false);
    energyBar->setStyleSheet(
        "QProgressBar { background-color: rgba(40, 30, 0, 150); border: 3px solid rgba(255, 165, 0, 80); border-radius: 2px; }"
        "QProgressBar::chunk { border-image: url(:/MainWindow/assets/images/MainWindow/bar_energy_chunk.png); width: 13px; margin: -1px 0px; }"
    );
    energyBar->setRange(0, 100);
    energyBar->setValue(75); // 视觉占位，后续可绑定真实蓄力系统

    energyLayout->addWidget(energyIcon);
    energyLayout->addWidget(energyText);
    energyLayout->addWidget(energyBar);
    energyLayout->addStretch(); // 顶满，不再放 Score

    // 将两行加入垂直布局
    barsLayout->addStretch();
    barsLayout->addWidget(hpRow);
    barsLayout->addWidget(energyRow);
    barsLayout->addStretch();

    // 组装头像和状态条
    leftLayout->addWidget(avatarLabel);
    leftLayout->addWidget(barsWidget);
    leftLayout->addStretch();

    // --- 3.2 中侧面板 (包含重定位的 Score) ---
    m_centerPanel = new QWidget(this);
    m_centerPanel->setObjectName("CenterPanel");
    // 增加细微的科技蓝描边和更深的半透明底色，强化 HUD 玻璃质感
    m_centerPanel->setStyleSheet("QWidget#CenterPanel { border-image: url(:/MainWindow/assets/images/MainWindow/panel_center.png) 15 15 15 15; background: rgba(10, 16, 28, 200); border: 1px solid rgba(0, 255, 204, 60); border-radius: 6px; }");

    QGridLayout* centerLayout = new QGridLayout(m_centerPanel);
    centerLayout->setContentsMargins(20, 10, 20, 10); // 微调内边距，使内容更紧凑[cite: 1]
    centerLayout->setSpacing(8);

    // 分数标签美化：采用高亮青蓝配色、Impact字体及斜体，增强视觉冲击力[cite: 1]
    m_scoreLabel = new QLabel("Score: 0", m_centerPanel);
    m_scoreLabel->setStyleSheet("color: #00FFFF; font-size: 24px; font-family: 'Impact', 'Consolas'; font-style: italic; background: transparent;");
    m_scoreLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // 替换原有的统计标签定义，翻译键值指向“总计”
    m_successLabel = new QLabel(tr("stats_total_destroys").arg(0), m_centerPanel);
    m_failLabel = new QLabel(tr("stats_total_escapes").arg(0), m_centerPanel);
    m_accuracyLabel = new QLabel(tr("stats_total_accuracy").arg(0), m_centerPanel);
    m_levelLabel = new QLabel(tr("stats_wave").arg(1), m_centerPanel);

    // 统计数据美化：采用冷灰蓝降低层级，突出 Score[cite: 1]
    QString statStyle = "color: #8AB4F8; font-size: 15px; font-family: 'Consolas', 'Microsoft YaHei'; background: transparent;";
    m_successLabel->setStyleSheet(statStyle);
    m_failLabel->setStyleSheet(statStyle);
    m_accuracyLabel->setStyleSheet(statStyle);
    m_levelLabel->setStyleSheet(statStyle);

    // 重新排版[cite: 1]
    centerLayout->addWidget(m_scoreLabel, 0, 0, 1, 2);
    centerLayout->addWidget(m_successLabel, 1, 0);
    centerLayout->addWidget(m_failLabel, 2, 0);
    centerLayout->addWidget(m_accuracyLabel, 1, 1);
    centerLayout->addWidget(m_levelLabel, 2, 1);

    // --- 3.3 右侧面板 ---
    m_rightPanel = new QWidget(this);
    m_rightPanel->setObjectName("RightPanel");
    m_rightPanel->setStyleSheet("QWidget#RightPanel { border-image: url(:/MainWindow/assets/images/MainWindow/panel_right.png) 15 15 15 15; background: transparent; }");

    // 给右侧面板添加水平布局，预留出透明边框的内边距，防止按钮贴边
    QHBoxLayout* rightLayout = new QHBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(20, 25, 20, 15);
    rightLayout->setSpacing(8);

    // 实例化所有的操作指针 (保持指针地址不变，确保后续 initConnections 正常工作)
    m_playPauseBtn = new QPushButton(m_rightPanel); // 原有的 m_startBtn 和 m_pauseBtn 合并为这个
    m_statsBtn = new QPushButton(m_rightPanel);
    m_settingsBtn = new QPushButton(m_rightPanel);
    m_exitBtn = new QPushButton(m_rightPanel);
    m_bagBtn = new QPushButton(m_rightPanel);
    m_upgradeBtn = new QPushButton(m_rightPanel);

    // 【核心黑魔法：Lambda 附魔器】
    // 将普通的 QPushButton 包装成 "六边形背景 + 中心Icon + 下方文字" 的复合控件
    auto morphIntoHexButton = [this](QPushButton* btn, const QString& iconName, const QString& text) -> QWidget* {
        QWidget* container = new QWidget(this);
        QVBoxLayout* vLayout = new QVBoxLayout(container);
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(5);

        // 接管传入的按钮，重新设置父级为当前的包裹容器
        btn->setParent(container);
        btn->setFixedSize(54, 60); // 设定六边形完美比例
        btn->setText("");          // 清除原生文字
        btn->setCursor(Qt::PointingHandCursor);

        // QSS 神技：border-image 画底框，image 绘制不拉伸的中心图标
        QString qss = QString(
            "QPushButton {"
            "   border-image: url(:/MainWindow/assets/images/MainWindow/btn_hex_normal.png);"
            "   image: url(:/MainWindow/assets/images/MainWindow/%1);"
            "   padding: 14px;" // 往内挤压，给六边形边框留出展示空间
            "   background: transparent;"
            "}"
            "QPushButton:hover {"
            "   border-image: url(:/MainWindow/assets/images/MainWindow/btn_hex_hover.png);"
            "}"
            "QPushButton:pressed {"
            "   padding: 16px;" // 按下时图标微微内缩，产生真实的物理按压感
            "}"
        ).arg(iconName);
        btn->setStyleSheet(qss);

        // 下方的科幻发光文字
        QLabel* label = new QLabel(text, container);
        label->setStyleSheet("color: #8AB4F8; font-size: 12px; font-weight: bold; background: transparent;");
        label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

        vLayout->addWidget(btn, 0, Qt::AlignCenter);
        vLayout->addWidget(label, 0, Qt::AlignCenter);
        vLayout->addStretch(); // 底部推挤，防止排版散架

        return container;
        };

    // 将组装好的复合按钮加入右侧布局 (使用标准的 tr() 宏翻译，拒绝中文硬编码)
    rightLayout->addStretch(); // 左侧加弹簧，整体居中靠拢
    rightLayout->addWidget(morphIntoHexButton(m_playPauseBtn, "icon_play.png", tr("start_game")));
    rightLayout->addWidget(morphIntoHexButton(m_statsBtn, "icon_chart.png", tr("menu_stats")));
    rightLayout->addWidget(morphIntoHexButton(m_settingsBtn, "icon_gear.png", tr("settings")));
    rightLayout->addWidget(morphIntoHexButton(m_exitBtn, "icon_exit.png", tr("exit_game")));
    rightLayout->addWidget(morphIntoHexButton(m_bagBtn, "icon_bag.png", tr("backpack")));
    rightLayout->addWidget(morphIntoHexButton(m_upgradeBtn, "icon_plane.png", tr("plane_attr")));
    rightLayout->addStretch(); // 右侧加弹簧

    bottomLayout->addWidget(m_leftPanel, 3);
    bottomLayout->addWidget(m_centerPanel, 3);
    bottomLayout->addWidget(m_rightPanel, 4);

    mainLayout->addWidget(m_bottomContainer, 0);
}

void PlaneGameWindow::initConnections()
{
    PlaneGameConfig* config = PlaneGameConfig::instance();

    // 按钮事件
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PlaneGameWindow::onPlayPauseClicked);
    connect(m_bagBtn, &QPushButton::clicked, this, &PlaneGameWindow::onBagClicked);
    connect(m_settingsBtn, &QPushButton::clicked, this, &PlaneGameWindow::onSettingsClicked);
    connect(m_exitBtn, &QPushButton::clicked, this, &PlaneGameWindow::onExitGame);

    // 配置数据更新
    connect(config, &PlaneGameConfig::statsChanged, this, &PlaneGameWindow::onStatsChanged);
    connect(config, &PlaneGameConfig::levelChanged, this, &PlaneGameWindow::onLevelChanged);
    connect(config, &PlaneGameConfig::scoreChanged, this, &PlaneGameWindow::onScoreChanged);

    // 新增：监听分辨率变更
    connect(config, &PlaneGameConfig::resolutionChanged, this, &PlaneGameWindow::onResolutionChanged);

    // 游戏状态联动
    connect(m_gameData, &PlaneGameData::stateChanged, this, &PlaneGameWindow::onGameStateChanged);
    connect(m_gameData, &GameDataBase::dataChanged, m_gameView, &PlaneGameView::onDataChanged);
    connect(m_gameData, &GameDataBase::stateChanged, m_gameView, &PlaneGameView::onStateChanged);

    connect(m_gameController, &PlaneGameController::exitGame, this, &PlaneGameWindow::onExitGame);
    connect(m_closeBtn, &QPushButton::clicked, this, &PlaneGameWindow::onExitGame);
}

void PlaneGameWindow::onPlayPauseClicked()
{
    GameDataBase::GameState state = m_gameData->getGameState();

    if (state == GameDataBase::Idle || state == GameDataBase::GameOver) {
        // 当前为未开始，执行开始游戏，并将按钮变为暂停状态
        AudioManager::instance()->playBackgroundMusic(AUDIO_BG_SPACE);
        m_gameController->startGame();
        updateHexButton(m_playPauseBtn, "icon_pause.png", tr("pause_game"));
        glWidget->setFocus();
    }
    else if (state == GameDataBase::Playing) {
        // 游戏中，执行暂停，并将按钮恢复为开始(继续)状态
        m_gameController->pauseGame();
        updateHexButton(m_playPauseBtn, "icon_play.png", tr("resume_game"));
        AudioManager::instance()->pauseBackgroundMusic();
    }
    else if (state == GameDataBase::Paused) {
        // 已暂停，执行恢复游戏，并将按钮变为暂停状态
        m_gameController->resumeGame();
        updateHexButton(m_playPauseBtn, "icon_pause.png", tr("pause_game"));
        AudioManager::instance()->resumeBackgroundMusic();
        glWidget->setFocus();
    }
}

void PlaneGameWindow::onBagClicked()
{
    bool wasPlaying = (m_gameData->getGameState() == GameDataBase::Playing);

    // 如果游戏正在进行，自动暂停，保证安全
    if (wasPlaying) {
        m_gameController->pauseGame();
        updateHexButton(m_playPauseBtn, "icon_play.png", tr("resume_game"));
        AudioManager::instance()->pauseBackgroundMusic();
    }

    // 实例化并弹窗展示背包界面（BackpackDialog 默认已是模态窗口）
    BackpackDialog bagDialog(this);
    bagDialog.exec();

    // 关闭背包后，自动恢复之前的游戏状态
    if (wasPlaying) {
        m_gameController->resumeGame();
        updateHexButton(m_playPauseBtn, "icon_pause.png", tr("pause_game"));
        AudioManager::instance()->resumeBackgroundMusic();
        glWidget->setFocus();
    }
}

void PlaneGameWindow::onStatsChanged(int success, int fail)
{
    // 获取游戏生命周期内的总击杀与逃脱数
    int totalSuccess = m_gameData->getTotalSuccessCount();
    int totalFail = m_gameData->getTotalFailCount();
    int total = totalSuccess + totalFail;

    // 计算总命中率
    int accuracy = total > 0 ? (totalSuccess * 100 / total) : 0;

    // 更新到底部中间面板
    m_successLabel->setText(tr("stats_total_destroys").arg(totalSuccess));
    m_failLabel->setText(tr("stats_total_escapes").arg(totalFail));
    m_accuracyLabel->setText(tr("stats_total_accuracy").arg(accuracy));
}

void PlaneGameWindow::onLevelChanged(int level)
{
    m_levelLabel->setText(tr("stats_level").arg(level));
}

void PlaneGameWindow::onGameStateChanged(int state)
{
    // 当游戏彻底结束 (GameOver)，或者点击再次游玩退回空闲状态 (Idle) 时
    // 统一将主控按钮恢复为“开始游戏”状态
    if (state == GameDataBase::GameOver || state == GameDataBase::Idle) {
        updateHexButton(m_playPauseBtn, "icon_play.png", tr("start_game"));
    }
}

void PlaneGameWindow::onSettingsClicked()
{
    bool wasPlaying = (m_gameData->getGameState() == GameDataBase::Playing);

    // 如果打开设置前游戏正在进行，自动暂停游戏，并更新按钮为“继续”状态
    if (wasPlaying) {
        m_gameController->pauseGame();
        updateHexButton(m_playPauseBtn, "icon_play.png", tr("resume_game"));
        AudioManager::instance()->pauseBackgroundMusic();
    }

    PlaneGameSettingsDialog settingsDialog(this);
    settingsDialog.exec();

    // 关闭设置对话框后，自动恢复游戏，并将按钮更新回“暂停”状态
    if (wasPlaying) {
        m_gameController->resumeGame();
        updateHexButton(m_playPauseBtn, "icon_pause.png", tr("pause_game"));
        AudioManager::instance()->resumeBackgroundMusic();
        glWidget->setFocus();
    }
}

void PlaneGameWindow::onExitGame()
{
    AudioManager::instance()->stopBackgroundMusic();
    close();
}

void PlaneGameWindow::onScoreChanged(int score)
{
    // 格式化为 6 位数字，不足补零，增加街机感
    m_scoreLabel->setText(QString("Score: %1").arg(score, 6, 10, QChar('0')));
}

void PlaneGameWindow::onPlayerHpChanged(float currentHp, float maxHp)
{
    // QProgressBar 使用整数，这里进行四舍五入或强制转换
    m_hpBar->setRange(0, static_cast<int>(maxHp));
    m_hpBar->setValue(static_cast<int>(currentHp));
    // 【核心黑魔法】：不修改头文件增加指针，直接通过 ObjectName 抓取 Label 更新文本
    QLabel* hpValueText = m_leftPanel->findChild<QLabel*>("HpValueText");
    if (hpValueText) {
        hpValueText->setText(QString("%1 / %2").arg(currentHp).arg(maxHp));
    }

    // 动态样式：血量低于 20% 时，切块变为危险的纯色猩红
    if (currentHp <= maxHp * 0.2f) {
        m_hpBar->setStyleSheet(
            "QProgressBar { background-color: rgba(40, 0, 0, 150); border: 3px solid #FF3333; border-radius: 2px; }"
            "QProgressBar::chunk { background-color: #FF5555; width: 13px; margin: -1px 0px; }"
        );
    }
    else {
        // 【核心修复】：使用 border-image
        m_hpBar->setStyleSheet(
            "QProgressBar { background-color: rgba(0, 20, 40, 150); border: 3px solid rgba(0, 191, 255, 80); border-radius: 2px; }"
            "QProgressBar::chunk { border-image: url(:/MainWindow/assets/images/MainWindow/bar_hp_chunk.png); width: 13px; margin: -1px 0px; }"
        );
    }
}

// 新增槽函数实现
void PlaneGameWindow::onResolutionChanged(int w, int h)
{
    // 【修改】：外框加上附加的UI高度 (200)
    this->resize(w, h + 200);
    // 【修改】：核心游戏视图精确对齐选项里的分辨率
    m_gameView->updateResolution(w, h);

    if (glWidget) {
        glWidget->updateGeometry();
    }
}

void PlaneGameWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 如果点击在标题栏范围内，允许拖拽
        if (m_titleBar && m_titleBar->geometry().contains(event->pos())) {
            m_isDragging = true;
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QMainWindow::mousePressEvent(event);
}

void PlaneGameWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton && m_isDragging) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QMainWindow::mouseMoveEvent(event);
}

void PlaneGameWindow::mouseReleaseEvent(QMouseEvent* event)
{
    m_isDragging = false;
    QMainWindow::mouseReleaseEvent(event);
}
