#include "planegameview.h"
#include "resourcemanager.h"
#include "entityfactory.h"  // 引入工厂用于销毁实体
#include "bulletentity.h"   // 引入追踪子弹实体类定义
#include "machinegunbulletentity.h"  // 引入机炮子弹实体类定义
#include <QDebug>


PlaneGameView::PlaneGameView(PlaneGameData* data, QObject* parent)
    : GameViewBase(data, parent)
{
    initExplosionResource(); // 预先加载并切割爆炸图片
    // 连接配置中心的关卡变化信号
    connect(getGameData()->getConfig(), &GameConfigBase::levelChanged, this, &PlaneGameView::onLevelChanged);
}

PlaneGameView::~PlaneGameView()
{
    clearScene();

    // 清理玩家战机
    if (m_playerEntity) {
        removeItem(m_playerEntity);
        delete m_playerEntity;
        m_playerEntity = nullptr;
    }

    // ========== 主动清理背景 OpenGL 资源 ==========
    if (m_shaderBackground) {
        // 主动调用，此时底层 glWidget 依然有效
        m_shaderBackground->cleanupGLResources();
        SAFE_DELETE(m_shaderBackground);
    }
    // ============================================================
}

void PlaneGameView::addBulletToScene(BulletEntity* bullet)
{
    if (!bullet || items().contains(bullet)) return;
    addItem(bullet);
}

void PlaneGameView::removeBulletFromScene(BulletEntity* bullet)
{
    if (!bullet) return;
    if (items().contains(bullet)) {
        removeItem(bullet);
    }
}

void PlaneGameView::addMachineGunBulletToScene(MachineGunBulletEntity* bullet)
{
    if (!bullet || items().contains(bullet)) return;
    addItem(bullet); 
}

void PlaneGameView::removeMachineGunBulletFromScene(MachineGunBulletEntity* bullet)
{
    if (!bullet) return;
    if (items().contains(bullet)) {
        removeItem(bullet);
    }
}

void PlaneGameView::playMissionSuccessAnimation()
{
    // 在屏幕中上方生成特效 (Y轴定位在偏上 1/3 处，最符合视觉习惯)
    QPointF centerPos(sceneRect().width() / 2.0, sceneRect().height() * 0.35);
    MissionSuccessTextItem* successText = new MissionSuccessTextItem(centerPos);
    addItem(successText);
}

void PlaneGameView::playBossWarningAnimation()
{
    QPointF centerPos(sceneRect().width() / 2.0, sceneRect().height() * 0.35);
    BossWarningTextItem* warningText = new BossWarningTextItem(centerPos);
    addItem(warningText);
}

void PlaneGameView::updateResolution(int width, int height)
{
    m_sceneWidth = width;
    m_sceneHeight = height;
    setSceneRect(0, 0, width, height);

    // 【关键】对齐 Model 层数据边界和失败线
    getGameData()->setSceneSize(width, height);
    getGameData()->setFailLineY(height + PlaneEntity::PLANE_HEIGHT);

    // 重载 Shader 背景以适配新分辨率
    if (m_shaderBackground) {
        m_shaderBackground->cleanupGLResources();
        removeItem(m_shaderBackground);
        delete m_shaderBackground;

        QString bgPath;
        QString shaderCode;
        if (m_currentStage == 0) {
            bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND.png";
            shaderCode = STAGE1_EARTH_SHADER;
        }
        else if (m_currentStage == 1) {
            bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND2.png";
            shaderCode = STAGE2_ZERG_SHADER;
        }
        else {
            bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND3.png";
            shaderCode = STAGE3_SPACE_SHADER;
        }

        QPixmap bgPixmap = ResourceManager::instance()->loadPixmap(bgPath);
        m_shaderBackground = new SingleImageBackground(width, height, bgPixmap, m_glWidget, shaderCode);
        addItem(m_shaderBackground);
        m_shaderBackground->setZValue(-100); // 置于最底层
    }

    // 如果玩家飞机存在，确保它被限制在新的屏幕最底部且不越界
    if (m_playerEntity) {
        int startY = height - PlayerEntity::PLAYER_HEIGHT - 20;
        m_playerEntity->setPos(qBound(0.0, m_playerEntity->x(), (double)width - PlayerEntity::PLAYER_WIDTH), startY);
    }
    update();
}

void PlaneGameView::onDataChanged(int dataType)
{
    switch (dataType) {
    case DATA_ENTITY_CHANGED:
        updateAllPlanes();
        break;
    case DATA_STATS_CHANGED:
        break;
    default:
        break;
    }
}

void PlaneGameView::onStateChanged(int state)
{
    // 1. 原有的清理逻辑
    if (state == GameDataBase::Idle || state == GameDataBase::GameOver) {
        clearScene();
        // 游戏结束/空闲时：停止玩家动画，清空按键状态防止卡死
        if (m_playerEntity) {
            m_playerEntity->setMovingDirection(0); // 确保战机姿态回到居中
        }
        m_isLeftPressed = false;
        m_isRightPressed = false;
        m_isSpacePressed = false; // 确保空格键状态也被重置
    }

    // ========== 修复：高性能弹窗漫画滚屏动画 ==========
    if (state == GameDataBase::LevelComplete || state == GameDataBase::GameOver) {
        if (!m_mangaScrollAnim) {
            m_mangaScrollAnim = new QVariantAnimation(this);
            connect(m_mangaScrollAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
                m_mangaScrollProgress = value.toReal();
                update(); // 触发 QGraphicsScene 前景层重绘

                // 【核心修复】：由于游戏主循环（Timer）在结算时已停止，
                // 必须显式通知底层的 OpenGL 代理控件进行画面重绘，否则画面会一直定格！
                if (m_glWidget) {
                    m_glWidget->update();
                }
                });
        }
        else {
            m_mangaScrollAnim->stop();
        }

        m_mangaScrollProgress = 0.0;
        m_mangaScrollAnim->setDuration(12000); // 加快到 12 秒一个来回，让运镜更明显
        m_mangaScrollAnim->setLoopCount(-1);   // 无限循环

        // 强制写入标准的浮点数 0.0 和 1.0，确保底层动画引擎正确进行 Double 插值
        m_mangaScrollAnim->setStartValue(0.0);
        m_mangaScrollAnim->setKeyValueAt(0.15, 0.0); // 顶部停留观察一段时间 (前 15% 时间)
        m_mangaScrollAnim->setKeyValueAt(0.70, 1.0); // 慢慢向下滚动到底部 (中间 55% 时间)
        m_mangaScrollAnim->setEndValue(1.0);         // 底部停留观察 (剩余 30% 时间)，循环时直接瞬间跳转回 0.0

        m_mangaScrollAnim->start();
    }
    else {
        // 离开结算界面，销毁资源
        if (m_mangaScrollAnim) {
            m_mangaScrollAnim->stop();
            m_mangaScrollAnim->deleteLater();
            m_mangaScrollAnim = nullptr;
        }
    }
    // =======================================================

    update();
}

// 【核心新增】触发关卡变化后的动态热替换
void PlaneGameView::onLevelChanged(int level)
{
    int newStage = 0;
    if (level >= 1 && level <= 3) newStage = 0;
    else if (level >= 4 && level <= 8) newStage = 1;
    else newStage = 2;

    // 只有跨越了阶段，才重新加载着色器和图片，彻底避免卡顿
    if (newStage != m_currentStage) {
        m_currentStage = newStage;

        QString bgPath;
        QString shaderCode;
        if (m_currentStage == 0) {
            bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND.png";
            shaderCode = STAGE1_EARTH_SHADER;
        }
        else if (m_currentStage == 1) {
            bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND2.png";
            shaderCode = STAGE2_ZERG_SHADER; 
        }
        else {
            bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND3.png";
            shaderCode = STAGE3_SPACE_SHADER;
        }

        QPixmap newBg = ResourceManager::instance()->loadPixmap(bgPath);
        if (m_shaderBackground && !newBg.isNull()) {
            m_shaderBackground->setPixmap(newBg);
            m_shaderBackground->setShaderCode(shaderCode);
        }
    }
}

void PlaneGameView::initScene(int width, int height)
{
    m_sceneWidth = width;
    m_sceneHeight = height;
    setSceneRect(0, 0, width, height);
    // ========== 【同步真实尺寸到数据层】 ==========
    getGameData()->setSceneSize(width, height);

    // 【核心新增/修改】：将失败线向下推移到屏幕外部
    // 屏幕高度 + 敌机高度，确保敌机必须完全飞出下方屏幕，底层才会判定为 Miss 并销毁
    getGameData()->setFailLineY(height + PlaneEntity::PLANE_HEIGHT);

    // 【修改】初始阶段判定
    int level = getGameData()->getConfig()->getLevel();
    if (level >= 1 && level <= 3) m_currentStage = 0;
    else if (level >= 4 && level <= 8) m_currentStage = 1;
    else m_currentStage = 2;

    // 映射3个阶段的背景图资源
    QString bgPath;
    QString shaderCode;
    if (m_currentStage == 0) {
        bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND.png"; // 敌占区背景
		shaderCode = STAGE1_EARTH_SHADER; // 废土战火、探照灯、硝烟
    }
    else if (m_currentStage == 1) {
        bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND2.png"; // 虫巢背景
		shaderCode = STAGE2_ZERG_SHADER;  // 紫色脉络、绿色毒气、剧毒孢子
    }
    else {
        bgPath = ":/SpaceWar/assets/images/SpaceWar/Plane_BACKGROUND3.png"; // 母舰背景
		shaderCode = STAGE3_SPACE_SHADER;  // 蓝色电磁网格、数据流、扫描线
    }

    QPixmap bgPixmap = ResourceManager::instance()->loadPixmap(bgPath);
    if (!bgPixmap.isNull()) {
        m_shaderBackground = new SingleImageBackground(width, height, bgPixmap, m_glWidget, shaderCode);
        addItem(m_shaderBackground);
    }

    clearScene();
    // =========================================================
    // 【新增】在这里调用初始化玩家飞机的方法！
    // 确保在 clearScene() 之后调用，否则刚创建就被清理逻辑影响了
    // =========================================================
	initPlayer(); // 初始化玩家战机
}

void PlaneGameView::clearScene()
{
    // 【安全清理场景中的所有实体】
    // 只要在场景里的实体，统统切断动画并回收
    QList<QGraphicsItem*> sceneItems = this->items();
    for (QGraphicsItem* item : sceneItems) {
        // 【核心修复】：使用基类 GameEntity 统一判断，防止奖励单词等新扩展实体被遗漏
        GameEntity* entity = dynamic_cast<GameEntity*>(item);

        // 【防穿模/消失保险】：必须排除玩家自身，玩家实体不由内存池管理且在下一局还要复用
        if (entity && entity != m_playerEntity) {
            // 由 Data 层管理的实体不在这里销毁，交由 Controller 统一管理生命周期
			const bool ownedByData = getGameData() && getGameData()->getAliveEntities().contains(entity); 
            // 切断信号连接，防止被回收后动画结束引发二次回收报错
            entity->disconnect(this);
            removeItem(entity); 
            // 视图层在这里只负责销毁已经被 Data 层剔除、正在播放淡出动画的废弃实体
            if (!entity->isAlive() && !ownedByData) {
                entity->destroy(); // 多态调用对应的 EntityFactory 内存池回收
            }
        }
    }
}

void PlaneGameView::drawLevelCompleteDialog(QPainter* painter)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // 1. 全屏战术遮罩
    painter->setBrush(QColor(8, 12, 20, 230));
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());

    // 2. 任务简报主面板区域
    qreal marginX = 40, marginY = 15;
    QRectF panelRect(marginX, marginY, m_sceneWidth - marginX * 2, m_sceneHeight - marginY * 2);

    painter->setBrush(QColor(15, 25, 40, 180));
    painter->setPen(QPen(QColor(0, 191, 255, 120), 2));
    painter->drawRect(panelRect);

    painter->setPen(QPen(QColor(0, 255, 204), 4));
    int cornerLen = 30;
    painter->drawLine(panelRect.topLeft(), panelRect.topLeft() + QPointF(cornerLen, 0));
    painter->drawLine(panelRect.topLeft(), panelRect.topLeft() + QPointF(0, cornerLen));
    painter->drawLine(panelRect.topRight(), panelRect.topRight() + QPointF(-cornerLen, 0));
    painter->drawLine(panelRect.topRight(), panelRect.topRight() + QPointF(0, cornerLen));
    painter->drawLine(panelRect.bottomLeft(), panelRect.bottomLeft() + QPointF(cornerLen, 0));
    painter->drawLine(panelRect.bottomLeft(), panelRect.bottomLeft() + QPointF(0, -cornerLen));
    painter->drawLine(panelRect.bottomRight(), panelRect.bottomRight() + QPointF(-cornerLen, 0));
    painter->drawLine(panelRect.bottomRight(), panelRect.bottomRight() + QPointF(0, -cornerLen));

    // 3. 左右布局划分
    qreal leftWidth = panelRect.width() * 0.52;
    QRectF leftRect(panelRect.left() + 10, panelRect.top() + 10, leftWidth, panelRect.height() - 20);
    QRectF rightRect(leftRect.right() + 40, panelRect.top() + 30, panelRect.width() - leftWidth - 70, panelRect.height() - 60);

    // 4. 左侧：加载漫画与剧情
    int currentLevel = getGameData()->getConfig()->getLevel();
    QString cgPath = QString(":/SpaceWar/assets/images/SpaceWar/CG_Stage%1.png").arg(currentLevel);
    QString storyTitle, storyText;

    switch (currentLevel) {
    case 1:
        storyTitle = tr("PHASE 01: AIRSPACE CLEARANCE");
        storyText = tr("Codename 'Blue Heron'. Arrived in the airspace above the enemy-occupied zone. Commencing clearance mission against enemy heavy fighters.");
        break;
    case 2:
        storyTitle = tr("PHASE 02: DECAPITATION STRIKE");
        storyText = tr("Executing Cobra Maneuver! Enemy commander's custom fighter destroyed.\n\nWarning: Extreme high-frequency unknown biological energy detected deep underground.");
        break;
    case 3:
        storyTitle = tr("PHASE 03: ABYSS DESCENT");
        storyText = tr("Diving into the abyss through the breached enemy base. The environment is mutating. Sensors show a massive, terrifying Zerg hive ahead.");
        break;
    case 4:
        storyTitle = tr("PHASE 04: SWARM ATTACK");
        storyText = tr("The hive is awakened! Countless grotesque Zerg creatures are swarming from all directions. We cannot let them reach the surface!");
        break;
    case 5:
        storyTitle = tr("PHASE 05: BLOOD ROAD");
        storyText = tr("The hive tunnels are expanding. Biomass structures glowing with eerie light. Firing heavy armor-piercing rounds to break through the massive flesh gate!");
        break;
    case 6:
        storyTitle = tr("PHASE 06: MOTHER QUEEN");
        storyText = tr("Breached the core chamber! A mountain-sized Mother Bug Queen roars, unleashing a barrage of highly corrosive acid. Shield capacity dropping!");
        break;
    case 7:
        storyTitle = tr("PHASE 07: FATAL BLOW");
        storyText = tr("Pushing engine to the absolute limit. Spiral maneuver engaged! Unleashing all remaining firepower directly into the Queen's unprotected core!");
        break;
    case 8:
        storyTitle = tr("PHASE 08: THE WORMHOLE");
        storyText = tr("The Mother Queen is dead, but a giant artificial metal ring is revealed beneath its corpse. It's an active interstellar wormhole. The true enemy awaits!");
        break;
    case 9:
        storyTitle = tr("PHASE 09: ZETA BATTLE");
        storyText = tr("Exiting the wormhole into the Zeta Sector. Facing a massive alien fleet and the colossal Zeta Mothership. Dancing through the dense laser crossfire!");
        break;
    case 10:
    default:
        storyTitle = tr("PHASE 10: SAVE THE EARTH");
        storyText = tr("Breached the Zeta Mothership's core reactor! The anti-matter bomb is deployed. The invaders are finished. Returning to our beautiful blue Earth.");
        break;
    }

    QPixmap cgPixmap = ResourceManager::instance()->loadPixmap(cgPath);
    if (!cgPixmap.isNull()) {
        // 【终极方案：动态纹理视口采样】
        qreal viewW = leftRect.width();
        qreal viewH = leftRect.height();
        qreal origW = cgPixmap.width();
        qreal origH = cgPixmap.height();

        // 计算在保持原图宽度的前提下，视口所需的原图高度
        qreal sourceH = origW * (viewH / viewW);

        if (sourceH >= origH) {
            // 情况A：图片高度不足以填满视窗 (或正好填满)，无需滚动。居中绘制。
            qreal scale = viewW / origW;
            qreal drawH = origH * scale;
            qreal offsetY = (viewH - drawH) / 2.0;
            QRectF targetRect(leftRect.left(), leftRect.top() + offsetY, viewW, drawH);
            painter->drawPixmap(targetRect, cgPixmap, cgPixmap.rect());
        }
        else {
            // 情况B：长条漫画，高度溢出。利用 QPainter 的底层视口映射实现丝滑滚动！
            qreal maxSourceY = origH - sourceH;
            qreal currentSourceY = maxSourceY * m_mangaScrollProgress;

            // 框选出原图上当前需要展示的那一截 (Source Rect)
            QRectF sourceRect(0, currentSourceY, origW, sourceH);
            // 完美投射到左侧的目标矩形内，无需任何物理缩放和裁剪！
            painter->drawPixmap(leftRect, cgPixmap, sourceRect);
        }

        painter->setPen(QPen(QColor(0, 255, 204, 150), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(leftRect);
    }

    // 5. 右侧：任务简报面板
    QFont titleFont("Consolas", 32, QFont::Bold, true);
    painter->setFont(titleFont);
    painter->setPen(QColor(0, 255, 204));
    QRectF titleRect(rightRect.left(), rightRect.top(), rightRect.width(), 50);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, tr("MISSION CLEAR"));

    QFont subTitleFont("Consolas", 14, QFont::Bold);
    painter->setFont(subTitleFont);
    painter->setPen(QColor(0, 191, 255));
    QRectF subTitleRect(rightRect.left(), titleRect.bottom(), rightRect.width(), 30);
    painter->drawText(subTitleRect, Qt::AlignLeft | Qt::AlignVCenter, storyTitle);

    QFont storyFont("Microsoft YaHei", 13, QFont::Normal);
    painter->setFont(storyFont);
    painter->setPen(QColor(220, 230, 255));
    QRectF storyRect(rightRect.left(), subTitleRect.bottom() + 15, rightRect.width(), 130);
    painter->drawText(storyRect, Qt::AlignLeft | Qt::TextWordWrap, storyText);

    qreal lineY = storyRect.bottom() + 20;
    painter->setPen(QPen(QColor(0, 191, 255, 100), 1, Qt::DashLine));
    painter->drawLine(QPointF(rightRect.left(), lineY), QPointF(rightRect.right(), lineY));

    // 提取本关详细数据
    int hp = getPlayer()->getHp();
    int maxHp = getPlayer()->getMaxHp(); // 最高HP配置读取为 18
    int sp = 75; // SP占位符，保持与底层UI相同的视觉进度
    int maxSp = 100;
    int levelKills = getGameData()->getConfig()->getSuccessCount();
    int levelEscapes = getGameData()->getConfig()->getFailCount();
    int totalLevel = levelKills + levelEscapes;
    int levelAccuracy = totalLevel > 0 ? (levelKills * 100 / totalLevel) : 0;

    // 构建详细的本关数据统计表
    QStringList briefStats = {
        tr("HP : %1 / %2").arg(hp).arg(maxHp),
        tr("SP : %1 / %2").arg(sp).arg(maxSp),
        tr("WAVE : %1").arg(currentLevel),
        tr("KILLS : %1").arg(levelKills),
        tr("ESCAPES: %1").arg(levelEscapes),
        tr("ACCURACY: %1%").arg(levelAccuracy)
    };

    // 渲染右下角文字
    QFont dataFont("Consolas", 14, QFont::Bold);
    painter->setFont(dataFont);
    painter->setPen(QColor(180, 200, 220));

    qreal statY = lineY + 10;
    for (const QString& stat : briefStats) {
        // 采用循环紧凑排版，确保不遮挡右下角的 Next Level 按钮
        painter->drawText(QRectF(rightRect.left(), statY, rightRect.width(), 25), Qt::AlignLeft | Qt::AlignVCenter, stat);
        statY += 25;
    }

    // 6. 右下角：下一关按钮
    qreal btnW = 220, btnH = 50;
    m_nextLevelBtnRect = QRectF(rightRect.right() - btnW, rightRect.bottom() - btnH, btnW, btnH);

    painter->setBrush(QColor(0, 191, 255, 40));
    painter->setPen(QPen(QColor(0, 191, 255), 2));
    painter->drawRect(m_nextLevelBtnRect);
    painter->setBrush(QColor(0, 191, 255));
    painter->setPen(Qt::NoPen);
    painter->drawPolygon(QPolygonF() << m_nextLevelBtnRect.bottomRight() << m_nextLevelBtnRect.bottomRight() + QPointF(-15, 0) << m_nextLevelBtnRect.bottomRight() + QPointF(0, -15));

    painter->setPen(Qt::white);
    QFont btnFont("Consolas", 16, QFont::Bold);
    painter->setFont(btnFont);
    painter->drawText(m_nextLevelBtnRect, Qt::AlignCenter, tr("NEXT MISSION >>"));

    painter->restore();
}

void PlaneGameView::drawGameOverDialog(QPainter* painter)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    int currentLevel = getGameData()->getConfig()->getLevel();
    bool isVictory = (getPlayer()->getHp() > 0 && currentLevel >= GameCommon::MAX_LEVEL);

    // 1. 全屏遮罩 (胜利为深空蓝，战败为致命红)
    painter->setBrush(isVictory ? QColor(5, 15, 30, 230) : QColor(25, 5, 5, 230));
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());

    // 2. 简报主面板区域
    qreal marginX = 40, marginY = 15;
    QRectF panelRect(marginX, marginY, m_sceneWidth - marginX * 2, m_sceneHeight - marginY * 2);

    painter->setBrush(isVictory ? QColor(15, 25, 40, 180) : QColor(40, 15, 15, 180));
    painter->setPen(QPen(isVictory ? QColor(0, 191, 255, 120) : QColor(255, 51, 51, 150), 2));
    painter->drawRect(panelRect);

    painter->setPen(QPen(isVictory ? QColor(0, 255, 204) : QColor(255, 100, 100), 4));
    int cornerLen = 30;
    painter->drawLine(panelRect.topLeft(), panelRect.topLeft() + QPointF(cornerLen, 0));
    painter->drawLine(panelRect.topLeft(), panelRect.topLeft() + QPointF(0, cornerLen));
    painter->drawLine(panelRect.topRight(), panelRect.topRight() + QPointF(-cornerLen, 0));
    painter->drawLine(panelRect.topRight(), panelRect.topRight() + QPointF(0, cornerLen));
    painter->drawLine(panelRect.bottomLeft(), panelRect.bottomLeft() + QPointF(cornerLen, 0));
    painter->drawLine(panelRect.bottomLeft(), panelRect.bottomLeft() + QPointF(0, -cornerLen));
    painter->drawLine(panelRect.bottomRight(), panelRect.bottomRight() + QPointF(-cornerLen, 0));
    painter->drawLine(panelRect.bottomRight(), panelRect.bottomRight() + QPointF(0, -cornerLen));

    // 3. 左右布局划分
    qreal leftWidth = panelRect.width() * 0.52;
    QRectF leftRect(panelRect.left() + 10, panelRect.top() + 10, leftWidth, panelRect.height() - 20);
    QRectF rightRect(leftRect.right() + 40, panelRect.top() + 30, panelRect.width() - leftWidth - 70, panelRect.height() - 60);

    // 4. 左侧：加载当前漫画
    QString cgPath = QString(":/SpaceWar/assets/images/SpaceWar/CG_Stage%1.png").arg(currentLevel);
    QPixmap cgPixmap = ResourceManager::instance()->loadPixmap(cgPath);
    if (!cgPixmap.isNull()) {
        // 同步应用高性能纹理视口映射
        qreal viewW = leftRect.width();
        qreal viewH = leftRect.height();
        qreal origW = cgPixmap.width();
        qreal origH = cgPixmap.height();

        qreal sourceH = origW * (viewH / viewW);

        if (sourceH >= origH) {
            qreal scale = viewW / origW;
            qreal drawH = origH * scale;
            qreal offsetY = (viewH - drawH) / 2.0;
            QRectF targetRect(leftRect.left(), leftRect.top() + offsetY, viewW, drawH);
            painter->drawPixmap(targetRect, cgPixmap, cgPixmap.rect());
        }
        else {
            qreal maxSourceY = origH - sourceH;
            qreal currentSourceY = maxSourceY * m_mangaScrollProgress;
            QRectF sourceRect(0, currentSourceY, origW, sourceH);
            painter->drawPixmap(leftRect, cgPixmap, sourceRect);
        }

        // 滤镜覆盖
        painter->setBrush(isVictory ? QColor(0, 191, 255, 30) : QColor(255, 0, 0, 40));
        painter->setPen(Qt::NoPen);
        painter->drawRect(leftRect);

        // 外框
        painter->setPen(QPen(isVictory ? QColor(0, 255, 204, 150) : QColor(255, 51, 51), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(leftRect);
    }

    // 5. 右侧：GAME OVER / VICTORY 标题与文案
    QFont titleFont("Consolas", 36, QFont::Bold, true);
    painter->setFont(titleFont);
    painter->setPen(isVictory ? QColor(0, 255, 204) : QColor(255, 51, 51));

    QString mainTitle = isVictory ? tr("MISSION ACCOMPLISHED") : tr("SYSTEM OFFLINE");
    QRectF titleRect(rightRect.left(), rightRect.top(), rightRect.width(), 60);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, mainTitle);

    QString storyText = isVictory
        ? tr("The anti-matter bomb successfully detonated, and the Zeta Mothership has completely disintegrated!\n\nCommand Center: Excellent job, Ace Pilot! The entire fleet salutes you. Welcome home to our beautiful blue Earth!")
        : tr("Vital signs are critical. The aircraft's structural integrity has suffered irreversible damage.\n\nMission aborted. Attempting final data uplink synchronization...");

    QFont storyFont("Microsoft YaHei", 12, QFont::Normal);
    painter->setFont(storyFont);
    painter->setPen(isVictory ? QColor(220, 240, 255) : QColor(255, 200, 200));
    QRectF storyRect(rightRect.left(), titleRect.bottom() + 10, rightRect.width(), 80);
    painter->drawText(storyRect, Qt::AlignLeft | Qt::TextWordWrap, storyText);

    qreal lineY = storyRect.bottom() + 20;
    painter->setPen(QPen(isVictory ? QColor(0, 191, 255, 100) : QColor(255, 51, 51, 100), 1, Qt::DashLine));
    painter->drawLine(QPointF(rightRect.left(), lineY), QPointF(rightRect.right(), lineY));

    PlaneGameData* data = getGameData();
    int totalSuccess = data->getTotalSuccessCount();
    int totalFail = data->getTotalFailCount();
    int total = totalSuccess + totalFail;
    int accuracy = total > 0 ? (totalSuccess * 100 / total) : 0;

    QFont dataFont("Consolas", 14, QFont::Bold);
    painter->setFont(dataFont);
    painter->setPen(isVictory ? QColor(0, 255, 204) : QColor(220, 180, 180));

    QStringList stats = {
        tr("FINAL SCORE  : %1").arg(PlaneGameConfig::instance()->getScore(), 6, 10, QChar('0')),
        tr("TARGETS HIT  : %1").arg(totalSuccess),
        tr("TARGETS MISS : %1").arg(totalFail),
        tr("HIT ACCURACY : %1%").arg(accuracy),
        tr("COMBAT TIME  : %1").arg(data->getFormattedTotalGameTime())
    };

    qreal statY = lineY + 20;
    for (const QString& stat : stats) {
        painter->drawText(QRectF(rightRect.left(), statY, rightRect.width(), 30), Qt::AlignLeft | Qt::AlignVCenter, stat);
        statY += 35;
    }

    // 6. 右下角：重启与退出按钮
    qreal btnW = 160, btnH = 50, btnSpacing = 20;
    m_restartBtnRect = QRectF(rightRect.right() - btnW * 2 - btnSpacing, rightRect.bottom() - btnH, btnW, btnH);
    m_exitBtnRect = QRectF(rightRect.right() - btnW, rightRect.bottom() - btnH, btnW, btnH);

    painter->setBrush(isVictory ? QColor(0, 191, 255, 40) : QColor(255, 51, 51, 40));
    painter->setPen(QPen(isVictory ? QColor(0, 191, 255) : QColor(255, 51, 51), 2));
    painter->drawRect(m_restartBtnRect);
    painter->setPen(Qt::white);
    QFont btnFont("Consolas", 16, QFont::Bold);
    painter->setFont(btnFont);
    painter->drawText(m_restartBtnRect, Qt::AlignCenter, isVictory ? tr("PLAY AGAIN") : tr("REBOOT"));

    painter->setBrush(QColor(100, 100, 100, 40));
    painter->setPen(QPen(QColor(150, 150, 150), 2));
    painter->drawRect(m_exitBtnRect);
    painter->setPen(Qt::white);
    painter->drawText(m_exitBtnRect, Qt::AlignCenter, isVictory ? tr("RETURN") : tr("ABORT"));

    painter->restore();
}


void PlaneGameView::initExplosionResource()
{
    // 1. 加载普通爆炸效果
    QPixmap sheetNormal(":/SpaceWar/assets/images/SpaceWar/Explosion5.png");
    if (!sheetNormal.isNull()) {
        int cols = 5; int rows = 5;
        int frameW = sheetNormal.width() / cols;
        int frameH = sheetNormal.height() / rows;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                m_explosionFramesNormal.append(sheetNormal.copy(x * frameW, y * frameH, frameW, frameH));
            }
        }
    }
    else {
        qWarning() << "Failed to load Normal Explosion Image!";
    }

    // 2. 【核心新增】加载太空风格爆炸效果
    QPixmap sheetSpace(":/SpaceWar/assets/images/SpaceWar/Explosion4.png"); 
    if (!sheetSpace.isNull()) {
        int cols = 5; int rows = 5; // 如果排版不同，请在这里修改行列数
        int frameW = sheetSpace.width() / cols;
        int frameH = sheetSpace.height() / rows;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                m_explosionFramesSpace.append(sheetSpace.copy(x * frameW, y * frameH, frameW, frameH));
            }
        }
    }
    else {
        qWarning() << "Failed to load Space Explosion Image!";
    }

    // 3. 【新增】加载异虫爆炸效果 (Stage 1)
    QPixmap sheetZerg(":/SpaceWar/assets/images/SpaceWar/Explosion6.png");
    if (!sheetZerg.isNull()) {
        int cols = 5; int rows = 5; // 如果异虫爆炸图排版不同，请在这里修改行列数
        int frameW = sheetZerg.width() / cols;
        int frameH = sheetZerg.height() / rows;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                m_explosionFramesZerg.append(sheetZerg.copy(x * frameW, y * frameH, frameW, frameH));
            }
        }
    }
    else {
        qWarning() << "Failed to load Zerg Explosion Image!";
    }
}

void PlaneGameView::playExplosionAt(const QPointF& pos, int stage)
{
    // 【核心修改】根据敌机所属的阶段，完美匹配对应的爆炸序列图
    const QVector<QPixmap>* selectedFrames = &m_explosionFramesNormal; // 默认 Stage 2

    if (stage == 1) {
        selectedFrames = &m_explosionFramesZerg; // Stage 1: 虫巢使用异虫爆炸
    }
    else if (stage == 0) {
        selectedFrames = &m_explosionFramesSpace; // Stage 0: 母舰使用太空爆炸
    }
    // 容错：如果图片没加载成功，直接返回
    if (selectedFrames->isEmpty()) return;

    // 1. 为本次爆炸创建一个独立的动画控制器
    SpriteAnimation* anim = new SpriteAnimation(this);
    for (const QPixmap& frame : *selectedFrames) {
        anim->addFrame(frame);
    }
    anim->setLoop(false);       // 爆炸只需播放一次
    anim->setFrameInterval(80); // 播放速度：每帧停留80毫秒

    // 2. 存入活跃列表
    ExplosionInstance inst;
    inst.pos = pos;
    inst.animation = anim;
    m_activeExplosions.append(inst);

    // 3. 巧妙利用 Lambda 表达式接管生命周期：每切一帧，通知场景重绘
    connect(anim, &SpriteAnimation::frameChanged, this, [this]() {
        this->update();
        });

    // 4. 播放完毕后：从列表中移除，并销毁动画对象（完美防止内存泄漏）
    connect(anim, &SpriteAnimation::animationFinished, this, [this, anim]() {
        for (int i = 0; i < m_activeExplosions.size(); ++i) {
            if (m_activeExplosions[i].animation == anim) {
                m_activeExplosions.removeAt(i);
                break;
            }
        }
        anim->deleteLater();
        this->update(); // 清除最后一帧的残留
        });

    // 5. 点火播放！
    anim->start();
}

void PlaneGameView::onPlaneFadeOutFinished()
{
    PlaneEntity* plane = qobject_cast<PlaneEntity*>(sender()); // 获取发送信号的飞机对象
    if (!plane) return;
    if (items().contains(plane)) {
        removeItem(plane); // 移除坏飞机图元
    }
    // 视觉动画彻底结束后，才通知数据层释放字母
    getGameData()->removeExistLetter(plane->getLetter());
    EntityFactory::instance()->destroyEntity<PlaneEntity>(plane);
}

void PlaneGameView::addPlaneToScene(PlaneEntity* plane)
{
    if (!plane || items().contains(plane)) return;
    addItem(plane);
}

void PlaneGameView::removePlaneFromScene(PlaneEntity* plane)
{
    if (!plane) return;

    // 在太空大战（复用拯救苹果）底层逻辑中：
    // plane->isAlive() == true  -> 玩家成功输入字母命中（好状态被移除）
    // plane->isAlive() == false -> 敌机掉落到底部失败线（坏状态被移除）

    if (plane->isAlive()) {
        // ==========================================
        // 【命中分支】：玩家在半空中输入正确字母击落敌机
        // ==========================================
        // 1. 计算中心点，触发爆炸动画！
        QPointF centerPos = plane->scenePos() + QPointF(plane->boundingRect().width() / 2.0, plane->boundingRect().height() / 2.0);
        
        playExplosionAt(centerPos, plane->getStage()); // 【修改】传递敌机的 Stage

        // 2. 【新增】：弹出酷炫的 BOOM！浮动文字特效
        BoomTextItem* boomText = new BoomTextItem(centerPos);
        addItem(boomText);

        // ==========================================
        // 3. 将敌机图元从场景中立即移除（让出位置给爆炸特效）
        removeItem(plane);
    }
    else {
        // 关键：使用 Qt::UniqueConnection 防止多次越界触发重复绑定
        connect(plane, &PlaneEntity::fadeOutFinished, this, &PlaneGameView::onPlaneFadeOutFinished, Qt::UniqueConnection);
        plane->startFadeOut(300);
    }
}

void PlaneGameView::updateAllPlanes()
{
    QList<PlaneEntity*> alivePlanes = getGameData()->getAlivePlanes();
    // 新增飞机到场景
    for (auto plane : alivePlanes) {
        if (!items().contains(plane)) {
            addPlaneToScene(plane);
        }
    }

    update();// 刷新所有飞机位置
}

void PlaneGameView::updateAnimations(float deltaTime)
{
    if (m_shaderBackground && getGameData()->getGameState() == GameDataBase::Playing) {
        // 使用 Controller 传来的真实 deltaTime 更新着色器背景滚动
        m_shaderBackground->updateFrameTime(deltaTime);
    }

}

void PlaneGameView::drawBackground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    painter->setRenderHint(QPainter::Antialiasing);
}

void PlaneGameView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    painter->setRenderHint(QPainter::Antialiasing);

    // ==========================================
    // 【新增】优先渲染爆炸特效层
    // ==========================================
    if (!m_activeExplosions.isEmpty()) {
        painter->save();
        // 【魔法：加法混合】过滤 JPG 的纯黑背景，只保留高亮的火焰！
        painter->setCompositionMode(QPainter::CompositionMode_Plus); 
        for (const ExplosionInstance& inst : m_activeExplosions) {
            QPixmap frame = inst.animation->getCurrentFrame();
            if (!frame.isNull()) {
                // 根据当前帧的宽高，让爆炸画面完美居中对准之前的敌机
                QRectF targetRect(inst.pos.x() - frame.width() / 2.0,
                    inst.pos.y() - frame.height() / 2.0,
                    frame.width(), frame.height());
                painter->drawPixmap(targetRect.toRect(), frame);
            }
        }
        painter->restore();
    }
    // ==========================================

    // 弹窗绘制移到前景层，在所有游戏元素之上，不会被覆盖
    GameDataBase::GameState state = getGameData()->getGameState();
    if (state == GameDataBase::LevelComplete) {
        drawLevelCompleteDialog(painter);
    }
    else if (state == GameDataBase::GameOver) {
        drawGameOverDialog(painter);
    }
}

void PlaneGameView::mousePressEvent(QGraphicsSceneMouseEvent* event)
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

void PlaneGameView::keyPressEvent(QKeyEvent* event)
{
    // 1. 屏蔽操作系统的长按连发事件（防止不断触发 Press）
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    // 2. 【核心修改】优先拦截方向键！拦截后直接 return，绝对不传给基类
    // 防止基类（打字游戏逻辑）将方向键误判，导致底层状态混乱或焦点丢失
    if (event->key() == Qt::Key_Left) {
        m_isLeftPressed = true;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right) {
        m_isRightPressed = true;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Up) {
        m_upPressed = true;
    }
    if (event->key() == Qt::Key_Down) {
        m_downPressed = true;
    }
    // ======== 新增：拦截空格按下 ========
    if (event->key() == Qt::Key_Space) {
        m_isSpacePressed = true;
        event->accept();
        return;
    }

    // 3. 如果不是方向键，再交给基类处理（例如 Esc 退出等快捷键）
    GameViewBase::keyPressEvent(event);
}

void PlaneGameView::keyReleaseEvent(QKeyEvent* event)
{
    // 1. 屏蔽操作系统的长按连发事件（防止不断触发 Press）
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    // 2. 真实松开按键，立即重置状态！拦截后直接 return
    if (event->key() == Qt::Key_Left) {
        m_isLeftPressed = false;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right) {
        m_isRightPressed = false;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Up) {
        m_upPressed = false;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Down) {
        m_downPressed = false;
        event->accept();
        return;
    }
    // ======== 新增：拦截空格抬起 ========
    if (event->key() == Qt::Key_Space) {
        m_isSpacePressed = false;
        event->accept();
        return;
    }
    // 3. 非方向键的抬起交给基类
    GameViewBase::keyReleaseEvent(event);
}

void PlaneGameView::focusOutEvent(QFocusEvent* event)
{
    // 如果你在按着方向键时弹出了弹窗，或者切到了其他软件
    // 强制把移动状态置为 false，让战机停下并回正
    m_isLeftPressed = false;
    m_isRightPressed = false;
    m_isSpacePressed = false; // ======== 新增：失去焦点时中断射击 ========
    if (m_playerEntity) {
		m_playerEntity->setMovingDirection(0); // 0 表示无左右移动的意图，战机会自动回正
    }
    GameViewBase::focusOutEvent(event);

}

void PlaneGameView::initPlayer()
{
    if (!m_playerEntity) {
        m_playerEntity = new PlayerEntity();

        // 【注意】这里假设你把 PlayerPlane.jpg 放到了 Qt 资源文件(.qrc)的 /res/ 目录下
        // 如果你的路径不同，请修改此处的字符串！
        m_playerEntity->loadTexture(":/SpaceWar/assets/images/SpaceWar/PlayerPlane.png");

        // 将战机图元加入图形场景
        addItem(m_playerEntity);

        // 设置玩家战机的初始坐标：水平居中，垂直在最底部（预留一点点边距会更好看）
        int startX = sceneRect().width() / 2 - PlayerEntity::PLAYER_WIDTH / 2;
        int startY = sceneRect().height() - PlayerEntity::PLAYER_HEIGHT - 20; // 底部留出20像素边距

        m_playerEntity->setPos(startX, startY);
    }

}
