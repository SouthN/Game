#include "applegameview.h"
#include "resourcemanager.h"
#include "entityfactory.h" // 【新增】引入工厂用于销毁实体
#include <QRandomGenerator>
#include <QDebug>

AppleGameView::AppleGameView(AppleGameData* data, QObject* parent)
    : GameViewBase(data, parent)
{

}

AppleGameView::~AppleGameView()
{
    clearScene();
    if (m_basketItem) {
        removeItem(m_basketItem);
        delete m_basketItem;
		m_basketItem = nullptr; // 避免悬空指针
    }
    // ========== 【核心修复：主动清理背景 OpenGL 资源】 ==========
    if (m_shaderBackground) {
        // 主动调用，此时底层 glWidget 依然有效
        m_shaderBackground->cleanupGLResources();
        SAFE_DELETE(m_shaderBackground);
    }
    // ============================================================
}

void AppleGameView::updateResolution(int width, int height)
{
    m_sceneWidth = width;
    m_sceneHeight = height;
    setSceneRect(0, 0, width, height);

    // 【关键】对齐 Model 层数据边界
    getGameData()->setSceneSize(width, height);

    // 重载 Shader 背景以适配新分辨率
    if (m_shaderBackground) {
        m_shaderBackground->cleanupGLResources();
        removeItem(m_shaderBackground);
        delete m_shaderBackground;

        QPixmap bgPixmap = ResourceManager::instance()->loadPixmap(":/SaveTheApple/assets/images/SaveTheApple/APPLE_BACKGROUND.png");
        if (!bgPixmap.isNull()) {
            m_shaderBackground = new SingleImageBackground(width, height, bgPixmap, m_glWidget, APPLE_FRAG_SHADER);
            addItem(m_shaderBackground);
            m_shaderBackground->setZValue(-100); // 置于最底层
        }
    }

    // 调用集中管理的布局更新，篮子和小苹果都会一次性严丝合缝地贴合右下角
    updateBasketAndIconsLayout();

    update();

}

void AppleGameView::onDataChanged(int dataType)
{
    switch (dataType) {
        case DATA_ENTITY_CHANGED:
            updateAllApples();
            break;
        case DATA_STATS_CHANGED:
            spawnSuccessAppleIcon();
            break;
        default:
            break;
    }
}

void AppleGameView::onStateChanged(int state)
{
    // 游戏状态变化时的视图处理
    if (state == GameDataBase::Idle || state == GameDataBase::GameOver) {
        clearScene();
    }
    update();
}

void AppleGameView::initScene(int width, int height)
{
    m_sceneWidth = width;
    m_sceneHeight = height;
    setSceneRect(0, 0, width, height);

    // ========== 【同步真实尺寸到数据层】 ==========
    getGameData()->setSceneSize(width, height);
    // =========================================================

    // ========== 初始化Shader动态背景，绑定OpenGL上下文 ==========
	ResourceManager* resMgr = ResourceManager::instance(); // 获取资源管理器实例
    QPixmap bgPixmap = resMgr->loadPixmap(":/SaveTheApple/assets/images/SaveTheApple/APPLE_BACKGROUND.png");
    if (!bgPixmap.isNull()) {
        // 【核心修改】传入所属的OpenGL控件
        m_shaderBackground = new SingleImageBackground(width, height, bgPixmap, m_glWidget, APPLE_FRAG_SHADER);
        addItem(m_shaderBackground);
    }

    initBasket();
    clearScene();
}

void AppleGameView::clearScene()
{
    // 【核心重构：安全清理场景中的所有苹果】
    // 无论是好苹果还是正在淡出的坏苹果，只要在场景里，统统切断动画并回收
    QList<QGraphicsItem*> sceneItems = this->items();
    for (QGraphicsItem* item : sceneItems) {
        AppleEntity* apple = dynamic_cast<AppleEntity*>(item);
        if (apple) {
            // 切断信号连接，防止被回收后动画结束引发二次回收报错
            apple->disconnect(this);
            removeItem(apple);
            // ========== 【核心修复：防 Double Free 崩溃】 ==========
            // 正在游戏中的好苹果 (isAlive == true) 仍记录在 Data 层的 m_aliveApples 中，
            // 它们会由 AppleGameData 的析构或 resetData 统一销毁。
            // 视图层在这里只负责销毁已经被 Data 层剔除、正在播放淡出动画的坏苹果 (isAlive == false)
            if (!apple->isAlive()) {
                EntityFactory::instance()->destroyEntity<AppleEntity>(apple);
            }
        }
    }
    clearSuccessAppleIcons();
}

void AppleGameView::initBasket()
{
    if (m_basketItem) return;

    // 只需要创建空图元并加到场景
    m_basketItem = new QGraphicsPixmapItem();
    m_basketItem->setZValue(10);
    addItem(m_basketItem);

    // 调用统一排版即可自动完成贴图缩放与坐标设置
    updateBasketAndIconsLayout();
}

void AppleGameView::updateBasketAndIconsLayout()
{
    if (!m_basketItem) return;

    // 1. 动态重新生成适配当前分辨率的篮子贴图
    QString basketPath = ":/SaveTheApple/assets/images/SaveTheApple/APPLE_BASKET.png";
    QPixmap basketPixmap = ResourceManager::instance()->loadPixmap(basketPath);
    if (!basketPixmap.isNull()) {
        m_basketItem->setPixmap(basketPixmap.scaled(
            (int)(m_sceneWidth * 0.2),
            (int)(m_sceneHeight * 0.15),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }

    // 2. 重新定位篮子到右下角
    QRectF basketRect = m_basketItem->boundingRect();
    m_basketItem->setPos(
        m_sceneWidth - basketRect.width() - 20,
        m_sceneHeight - basketRect.height() - 20
    );

    // 3. 重新计算所有小苹果的排列
    qreal basketRightX = m_basketItem->x() + basketRect.width();
    qreal basketTopY = m_basketItem->y() + 80;
    const int MAX_PER_ROW = 5;
    qreal iconStep = SMALL_APPLE_WIDTH + SMALL_APPLE_SPACING;
    qreal totalRowWidth = MAX_PER_ROW * iconStep - SMALL_APPLE_SPACING;
    qreal availableWidth = basketRightX - 20;
    qreal scaleX = qMin(1.0, availableWidth / totalRowWidth);

    // 遍历现存的所有小苹果，统一刷新坐标
    for (int i = 0; i < m_successAppleIcons.size(); ++i) {
        QGraphicsPixmapItem* icon = m_successAppleIcons[i];
        if (!icon) continue;
        int row = i / MAX_PER_ROW;
        int col = i % MAX_PER_ROW;
        qreal iconX = basketRightX - (col * iconStep * scaleX) - SMALL_APPLE_WIDTH * scaleX;
        qreal iconY = basketTopY - (row + 1) * (SMALL_APPLE_HEIGHT + SMALL_APPLE_SPACING) - 10;
        icon->setPos(iconX, iconY);
    }
}

void AppleGameView::drawLevelCompleteDialog(QPainter* painter)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    // 半透明晨雾遮罩
    painter->setBrush(QColor(0, 30, 10, 120));
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());

    // 弹窗磨砂背景
    qreal dialogW = 400, dialogH = 300;
    QRectF dialogRect((m_sceneWidth - dialogW) / 2, (m_sceneHeight - dialogH) / 2, dialogW, dialogH);
    // 晨雾白半透明背景
    painter->setBrush(QColor(248, 245, 240, 240));
    // 晨光金细边框
    painter->setPen(QPen(QColor(230, 184, 92, 150), 2));
    painter->drawRoundedRect(dialogRect, 16, 16);
    // 柔和阴影
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 30));
    painter->drawRoundedRect(dialogRect.adjusted(4, 4, 4, 4), 16, 16);

    // 标题
    QFont titleFont = painter->font();
    titleFont.setPixelSize(32);
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(QColor(74, 124, 89));
    QString title = tr("level_complete_title");
    QRectF titleRect = painter->boundingRect(dialogRect, Qt::AlignTop | Qt::AlignHCenter, title);
    titleRect.moveTop(dialogRect.top() + 30);
    painter->drawText(titleRect, Qt::AlignCenter, title);

    // 关卡信息
    QFont infoFont = painter->font();
    infoFont.setPixelSize(20);
    infoFont.setBold(false);
    painter->setFont(infoFont);
    painter->setPen(QColor(45, 62, 51));
    AppleGameConfig* config = getGameData()->getConfig();
    QString levelText = tr("level_passed_text").arg(config->getLevel());
    QRectF levelRect = painter->boundingRect(dialogRect, Qt::AlignCenter, levelText);
    levelRect.moveTop(titleRect.bottom() + 30);
    painter->drawText(levelRect, Qt::AlignCenter, levelText);

    // 下一关按钮
    qreal btnW = 140, btnH = 48;
    m_nextLevelBtnRect = QRectF((m_sceneWidth - btnW) / 2, dialogRect.bottom() - 70, btnW, btnH);
    // 按钮背景
    painter->setBrush(QColor(74, 124, 89, 230));
    painter->setPen(QPen(QColor(255, 255, 255, 100), 1));
    painter->drawRoundedRect(m_nextLevelBtnRect, 10, 10);
    // 按钮文字
    painter->setPen(Qt::white);
    QFont btnFont = painter->font();
    btnFont.setPixelSize(16);
    btnFont.setBold(true);
    painter->setFont(btnFont);
    painter->drawText(m_nextLevelBtnRect, Qt::AlignCenter, tr("btn_next_level"));

    painter->restore();
}

void AppleGameView::drawGameOverDialog(QPainter* painter)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    // 半透明晨雾遮罩
    painter->setBrush(QColor(0, 30, 10, 120));
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());

    // 弹窗磨砂背景
    qreal dialogW = 450, dialogH = 420;
    QRectF dialogRect((m_sceneWidth - dialogW) / 2, (m_sceneHeight - dialogH) / 2, dialogW, dialogH);
    painter->setBrush(QColor(248, 245, 240, 240));
    painter->setPen(QPen(QColor(230, 184, 92, 150), 2));
    painter->drawRoundedRect(dialogRect, 16, 16);
    // 柔和阴影
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 30));
    painter->drawRoundedRect(dialogRect.adjusted(4, 4, 4, 4), 16, 16);

    // 标题
    QFont titleFont = painter->font();
    titleFont.setPixelSize(32);
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(QColor(74, 124, 89));
    QString title = tr("game_over_congratulations");
    QRectF titleRect = painter->boundingRect(dialogRect, Qt::AlignTop | Qt::AlignHCenter, title);
    titleRect.moveTop(dialogRect.top() + 30);
    painter->drawText(titleRect, Qt::AlignCenter, title);

    // 统计数据
    AppleGameData* data = getGameData();
    AppleGameConfig* config = data->getConfig();
    int totalSuccess = data->getTotalSuccessCount();
    int totalFail = data->getTotalFailCount();
    int total = totalSuccess + totalFail;
    int accuracy = total > 0 ? (totalSuccess * 100 / total) : 0;
    QString timeText = data->getFormattedTotalGameTime();

    QFont infoFont = painter->font();
    infoFont.setPixelSize(18);
    infoFont.setBold(false);
    painter->setFont(infoFont);
    painter->setPen(QColor(45, 62, 51));

    QStringList stats;
    stats << tr("stats_total_success").arg(totalSuccess)
        << tr("stats_total_fail").arg(totalFail)
        << tr("stats_total_accuracy").arg(accuracy)
        << tr("stats_total_time").arg(timeText);

    qreal y = titleRect.bottom() + 40;
    for (const QString& stat : stats) {
        QRectF statRect = painter->boundingRect(dialogRect, Qt::AlignCenter, stat);
        statRect.moveTop(y);
        painter->drawText(statRect, Qt::AlignCenter, stat);
        y += 40;
    }

    // 按钮
    qreal btnW = 120, btnH = 48, btnSpacing = 30;
    qreal totalBtnW = btnW * 2 + btnSpacing;
    qreal btnX = (m_sceneWidth - totalBtnW) / 2;
    m_restartBtnRect = QRectF(btnX, dialogRect.bottom() - 70, btnW, btnH);
    m_exitBtnRect = QRectF(btnX + btnW + btnSpacing, dialogRect.bottom() - 70, btnW, btnH);

    // 重新开始按钮
    painter->setBrush(QColor(74, 124, 89, 230));
    painter->setPen(QPen(QColor(255, 255, 255, 100), 1));
    painter->drawRoundedRect(m_restartBtnRect, 10, 10);
    painter->setPen(Qt::white);
    QFont btnFont = painter->font();
    btnFont.setPixelSize(16);
    btnFont.setBold(true);
    painter->setFont(btnFont);
    painter->drawText(m_restartBtnRect, Qt::AlignCenter, tr("btn_restart_game"));

    // 退出按钮
    painter->setBrush(QColor(168, 92, 56, 230));
    painter->setPen(QPen(QColor(255, 255, 255, 100), 1));
    painter->drawRoundedRect(m_exitBtnRect, 10, 10);
    painter->setPen(Qt::white);
    painter->drawText(m_exitBtnRect, Qt::AlignCenter, tr("btn_exit_game"));

    painter->restore();
}

void AppleGameView::onAppleFadeOutFinished()
{
	AppleEntity* apple = qobject_cast<AppleEntity*>(sender()); // 获取发送信号的苹果对象
    if (!apple) return;
    if (items().contains(apple)) {
		removeItem(apple); // 移除坏苹果图元
    }
    // 【核心修复 2】：视觉动画彻底结束后，才通知数据层释放字母
    getGameData()->removeExistLetter(apple->getLetter());

    EntityFactory::instance()->destroyEntity<AppleEntity>(apple);
}

void AppleGameView::addAppleToScene(AppleEntity* apple)
{
    if (!apple || items().contains(apple)) return;
    addItem(apple);
}

void AppleGameView::removeAppleFromScene(AppleEntity* apple)
{
    if (!apple) return;
    // 核心修复：基于生命周期触发动画
    if (!apple->isAlive()) {
        // 关键：使用 Qt::UniqueConnection 防止多次越界触发重复绑定
        connect(apple, &AppleEntity::fadeOutFinished, this, &AppleGameView::onAppleFadeOutFinished, Qt::UniqueConnection);
        apple->startFadeOut(300);
    } else {
		removeItem(apple);
    }
}

void AppleGameView::updateAllApples()
{
    QList<AppleEntity*> aliveApples = getGameData()->getAliveApples();
    // 新增苹果到场景
    for (auto apple : aliveApples) {
        if (!items().contains(apple)) {
            addAppleToScene(apple);
        }
    }

    // ========== 更新Shader背景动画 ==========
    if (m_shaderBackground) {
        // 游戏运行时更新背景动画
        GameDataBase::GameState state = getGameData()->getGameState();
        if (state == GameDataBase::Playing) {
            m_shaderBackground->updateFrameTime(16.0 / 1000.0);
        }
    }

    // 刷新所有苹果位置
    update();
}

void AppleGameView::spawnSuccessAppleIcon()
{
    if (!m_basketItem) return;
    QString smallApplePath = ":/SaveTheApple/assets/images/SaveTheApple/APPLE_REAL.png";
    QPixmap smallApplePixmap = ResourceManager::instance()->loadPixmap(smallApplePath);
    if (smallApplePixmap.isNull()) return;

    QPixmap scaledPixmap = smallApplePixmap.scaled(
        SMALL_APPLE_WIDTH,
        SMALL_APPLE_HEIGHT,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    
    // 创建图标并加入列表
    QGraphicsPixmapItem* appleIcon = new QGraphicsPixmapItem(scaledPixmap);
    appleIcon->setZValue(11);
    addItem(appleIcon);
    m_successAppleIcons.append(appleIcon);

    // 调用统一排版，它会自动为刚加进来的小苹果算出正确的坐标
    updateBasketAndIconsLayout();
}

void AppleGameView::clearSuccessAppleIcons()
{
    for (auto icon : m_successAppleIcons) {
        if (icon && items().contains(icon)) {
            removeItem(icon);
			delete icon; // 直接删除图元对象，释放内存
        }
    }
    m_successAppleIcons.clear();
}

void AppleGameView::drawBackground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
}

void AppleGameView::drawForeground(QPainter* painter, const QRectF& rect) 
{
    Q_UNUSED(rect);
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制70%高度的失败线
    qreal failLineY = getGameData()->getFailLineY();
    painter->setPen(QPen(QColor(255, 255, 255, 100), 2, Qt::DashLine));
    painter->drawLine(QPointF(0, failLineY), QPointF(m_sceneWidth, failLineY));

    // 【核心修复】弹窗绘制移到前景层，在所有游戏元素之上，不会被覆盖
    GameDataBase::GameState state = getGameData()->getGameState();
    if (state == GameDataBase::LevelComplete) {
        drawLevelCompleteDialog(painter);
    }
    else if (state == GameDataBase::GameOver) {
        drawGameOverDialog(painter);
    }
}

void AppleGameView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    GameDataBase::GameState state = getGameData()->getGameState();
    QPointF clickPos = event->scenePos();

    if (state == GameDataBase::LevelComplete) {
        if (m_nextLevelBtnRect.contains(clickPos)) {
            emit nextLevelClicked();
        }
    }
    else if (state == GameDataBase::GameOver) {
        if (m_restartBtnRect.contains(clickPos)) {
            emit restartGameClicked();
        }
        else if (m_exitBtnRect.contains(clickPos)) {
            emit exitGameClicked();
        }
    }

    GameViewBase::mousePressEvent(event);

}
