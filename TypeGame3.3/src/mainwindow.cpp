#include "mainWindow.h"
#include "Games/SaveTheApple/applegamewindow.h"
#include "Games/SpaceWar/planegamewindow.h" 
#include "Core/resourcemanager.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QStyle>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Typing Game Launcher"));
    resize(1200, 800);
    initUI();
}

MainWindow::~MainWindow() {}

void MainWindow::initUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    initTopBar();
    initNavBar();
    initGameArea();
    initBottomBar();
    mainLayout->addWidget(m_topBar);
    mainLayout->addWidget(m_navBar);
    mainLayout->addWidget(m_gameArea, 1);
    mainLayout->addWidget(m_bottomBar);
}

void MainWindow::initTopBar()
{
    m_topBar = new QWidget(this);
    m_topBar->setObjectName("TopBar");
    QHBoxLayout* topLayout = new QHBoxLayout(m_topBar);
    topLayout->setContentsMargins(20, 0, 10, 0);
    topLayout->setSpacing(10);

    // 左上角Logo加载与显示（核心新增修改部分）
    QLabel* logoLabel = new QLabel(m_topBar);
    logoLabel->setFixedSize(80, 80);
    logoLabel->setAlignment(Qt::AlignCenter);
    // 加载qrc中的Logo资源，复用项目统一的ResourceManager
    QPixmap logoPixmap = ResourceManager::instance()->loadPixmap(":/MainWindow/assets/images/MainWindow/logo.png");
    // 图片非空则做适配缩放，保持宽高比+平滑缩放
    if (!logoPixmap.isNull()) {
        logoPixmap = logoPixmap.scaled(
            logoLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        logoLabel->setPixmap(logoPixmap);
    }

    QLabel* titleLabel = new QLabel(tr("typing_game_launcher"), m_topBar);
    titleLabel->setObjectName("TitleLabel");

    topLayout->addWidget(logoLabel);
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    QPushButton* loginBtn = new QPushButton(tr("login"), m_topBar);
    loginBtn->setObjectName("LoginBtn");
    topLayout->addWidget(loginBtn);

    // 窗口控制按钮
    QPushButton* minBtn = new QPushButton("-", m_topBar);
    minBtn->setObjectName("WindowCtrlBtn");
    QPushButton* maxBtn = new QPushButton("[]", m_topBar);
    maxBtn->setObjectName("WindowCtrlBtn");
    QPushButton* closeBtn = new QPushButton("X", m_topBar);
    closeBtn->setObjectName("WindowCtrlBtn");

    topLayout->addWidget(minBtn);
    topLayout->addWidget(maxBtn);
    topLayout->addWidget(closeBtn);

    connect(minBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(maxBtn, &QPushButton::clicked, this, [this, maxBtn]() {
        if (isMaximized()) {
            showNormal();
            maxBtn->setText("[]");
        }
        else {
            showMaximized();
            maxBtn->setText("R");
        }
        });

    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);
}

void MainWindow::initNavBar()
{
    m_navBar = new QWidget(this);
    m_navBar->setObjectName("NavBar");
    QHBoxLayout* navLayout = new QHBoxLayout(m_navBar);
    navLayout->setContentsMargins(20, 0, 20, 0);
    navLayout->setSpacing(10);

    QPushButton* backBtn = new QPushButton(tr("back"), m_navBar);
    backBtn->setObjectName("BackBtn");
    QLabel* breadcrumbLabel = new QLabel(tr("home_games_breadcrumb"), m_navBar);
    breadcrumbLabel->setObjectName("BreadcrumbLabel");

    navLayout->addWidget(backBtn);
    navLayout->addWidget(breadcrumbLabel);
    navLayout->addStretch();
}

void MainWindow::initGameArea()
{
    m_gameArea = new QWidget(this);
    QVBoxLayout* gameMainLayout = new QVBoxLayout(m_gameArea);
    gameMainLayout->setContentsMargins(20, 10, 20, 20);
    gameMainLayout->setSpacing(10);

    QLabel* categoryLabel = new QLabel(tr("classic_games"), m_gameArea);
    categoryLabel->setObjectName("CategoryLabel");
    gameMainLayout->addWidget(categoryLabel);

    QGridLayout* gameGridLayout = new QGridLayout();
    gameGridLayout->setSpacing(30);
    gameGridLayout->setContentsMargins(0, 0, 0, 0);

    // 游戏列表完全保留原有配置
    gameInfos = {
        {tr("game_life_death_speed"), ":/SaveTheApple/assets/images/SaveTheApple/logo2.jpg"},
        {tr("game_mouse_story"), ":/SaveTheApple/assets/images/SaveTheApple/logo2.jpg"},
        {tr("game_save_the_apple"), ":/SaveTheApple/assets/images/SaveTheApple/logo2.jpg"},
        {tr("game_space_war"), ":/SpaceWar/assets/images/SpaceWar/logo.png"},
        {tr("game_rapids_advance"), ":/SaveTheApple/assets/images/SaveTheApple/logo2.jpg"}
    };

    for (int i = 0; i < gameInfos.size(); ++i) {
        QPushButton* gameCard = new QPushButton(m_gameArea);
        gameCard->setObjectName("GameCard");
        gameCard->setFixedSize(200, 220);
        gameCard->setProperty("gameIndex", i);
        gameCard->setCheckable(true);
        m_gameCardList.append(gameCard);

        QVBoxLayout* cardLayout = new QVBoxLayout(gameCard);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(8);

        // 游戏图标
        QLabel* gameIcon = new QLabel(gameCard);
        gameIcon->setObjectName("GameIcon");
        gameIcon->setFixedSize(180, 160);
        gameIcon->setAlignment(Qt::AlignCenter);
        gameIcon->setAttribute(Qt::WA_TransparentForMouseEvents);

        // 资源加载完全保留原有逻辑
        QPixmap gamePixmap;
        const QString& iconPath = gameInfos[i].iconPath;
        if (!iconPath.isEmpty()) {
            gamePixmap = ResourceManager::instance()->loadPixmap(iconPath);
        }
        if (!gamePixmap.isNull()) {
            gamePixmap = gamePixmap.scaled(
                gameIcon->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );
            gameIcon->setPixmap(gamePixmap);
        }

        // 游戏名称
        QLabel* gameNameLabel = new QLabel(gameInfos[i].name, gameCard);
        gameNameLabel->setObjectName("GameName");
        gameNameLabel->setAlignment(Qt::AlignCenter);
        gameNameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        cardLayout->addWidget(gameIcon);
        cardLayout->addWidget(gameNameLabel);
        gameGridLayout->addWidget(gameCard, 0, i);

        connect(gameCard, &QPushButton::clicked, this, &MainWindow::onGameCardClicked);
        if (i != 2 && i != 3) gameCard->setEnabled(false);
    }

    gameMainLayout->addLayout(gameGridLayout);
    gameMainLayout->addStretch();
}

void MainWindow::onGameCardClicked()
{
    QPushButton* clickedCard = qobject_cast<QPushButton*>(sender());
    if (!clickedCard) return;
    int gameIndex = clickedCard->property("gameIndex").toInt();

    // 重置选中状态
    for (QWidget* card : m_gameCardList) {
        QPushButton* btn = qobject_cast<QPushButton*>(card);
        if (btn) {
            btn->setChecked(false);
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);
        }
    }

    clickedCard->setChecked(true);
    clickedCard->style()->unpolish(clickedCard);
    clickedCard->style()->polish(clickedCard);

    if (gameIndex == 2) onAppleGameClicked();
    else if (gameIndex == 3) onSpaceWarClicked();
}

void MainWindow::initBottomBar()
{
    m_bottomBar = new QWidget(this);
    m_bottomBar->setObjectName("BottomBar");
    QHBoxLayout* bottomLayout = new QHBoxLayout(m_bottomBar);
    bottomLayout->setContentsMargins(0, 0, 20, 0);
    bottomLayout->addStretch();

    QPushButton* settingBtn = new QPushButton(tr("settings"), m_bottomBar);
    settingBtn->setObjectName("SettingBtn");
    bottomLayout->addWidget(settingBtn);
}

// 窗口拖动事件完全保留原有逻辑
void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_topBar->underMouse()) {
        m_isDragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    m_isDragging = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::onAppleGameClicked()
{
    AppleGameWindow* gameWindow = new AppleGameWindow(this);
    gameWindow->setAttribute(Qt::WA_DeleteOnClose);
    gameWindow->show();
}

void MainWindow::onSpaceWarClicked()
{

    PlaneGameWindow* gameWindow = new PlaneGameWindow(this);
    gameWindow->setAttribute(Qt::WA_DeleteOnClose);
    gameWindow->show();

}