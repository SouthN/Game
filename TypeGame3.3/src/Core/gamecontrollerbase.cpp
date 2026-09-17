#include "gamecontrollerbase.h"
#include "inpututils.h"
#include "logmanager.h"
#include "analyticsmanager.h"
#include <QRandomGenerator>

GameControllerBase::GameControllerBase(GameDataBase* data, GameViewBase* view, QObject* parent)
    : QObject(parent), m_gameData(data), m_gameView(view)
{
    if (m_gameData) {
        connect(m_gameData, &GameDataBase::dataChanged, this, &GameControllerBase::onDataChanged);
        connect(m_gameData, &GameDataBase::stateChanged, this, &GameControllerBase::onStateChanged);

        // 自动绑定关卡变化以更新速度
        if (m_gameData->getConfig()) {
            connect(m_gameData->getConfig(), &GameConfigBase::levelChanged, this, &GameControllerBase::onLevelChanged);
        }
    }
    initGameLoop();
}

GameControllerBase::~GameControllerBase()
{
    if (m_gameLoopTimer) {
        m_gameLoopTimer->stop();
        delete m_gameLoopTimer;
    }
}

void GameControllerBase::initGameLoop()
{
    m_gameLoopTimer = new QTimer(this);
    m_gameLoopTimer->setInterval(GAME_LOOP_INTERVAL);
    connect(m_gameLoopTimer, &QTimer::timeout, this, &GameControllerBase::onGameLoop);
}

void GameControllerBase::startGame()
{
    LOG_INFO(tr("Game started, level: %1").arg(m_gameData->getConfig()->getLevel()));
    ANALYTICS_EVENT("game_start", (QJsonObject{ {"level", m_gameData->getConfig()->getLevel()} }));

    if (m_gameData->getGameState() == GameDataBase::Playing) return;

    m_gameData->resetData();
    if (m_gameView) m_gameView->clearScene();
    m_gameData->setGameState(GameDataBase::Playing);

    if (!m_isTestMode) m_gameLoopTimer->start();
}

void GameControllerBase::pauseGame()
{
    if (m_gameData->getGameState() != GameDataBase::Playing) return;
    m_gameData->setGameState(GameDataBase::Paused);
    m_gameLoopTimer->stop();
}

void GameControllerBase::resumeGame()
{
    if (m_gameData->getGameState() != GameDataBase::Paused) return;
    m_gameData->setGameState(GameDataBase::Playing);
    if (!m_isTestMode) m_gameLoopTimer->start();
}

void GameControllerBase::stopGame()
{
    if (m_gameData->getGameState() == GameDataBase::Idle) return;
    m_gameLoopTimer->stop();
    if (m_gameView) m_gameView->clearScene();
    m_gameData->setGameState(GameDataBase::Idle);

    // 重置配置状态
    m_gameData->getConfig()->setLevel(GameCommon::MIN_LEVEL);
    m_gameData->getConfig()->resetStats();
}

void GameControllerBase::onStateChanged(int state)
{
    if (state == GameDataBase::Playing) {
        m_frameTimer.start();
    }
}

void GameControllerBase::onGameLoop()
{
    if (m_gameData->getGameState() != GameDataBase::Playing) return;

    float frameTime = GAME_LOOP_INTERVAL / 1000.0f;

    if (!m_isTestMode) {
        if (m_frameTimer.isValid()) {
            frameTime = m_frameTimer.restart() / 1000.0f;
            // 【防螺旋死亡/穿模限制】即使发生了极大卡顿，单帧最多只补偿 0.1 秒的物理时间
            if (frameTime > 0.1f) frameTime = 0.1f;
        }
        else {
            m_frameTimer.start();
        }
    }

    // 将本帧真实的流逝时间加入累加器
    m_accumulator += frameTime;

    // 【核心重构：固定步长物理循环】
    // 只要累加器里还剩下超过 16ms 的时间，就执行一次严格的物理计算
    while (m_accumulator >= FIXED_TIME_STEP) {
        // 1. 基础实体的物理下落
        QList<GameEntity*> aliveEntities = m_gameData->getAliveEntities();
        for (auto entity : aliveEntities) {
            entity->updateLogic(FIXED_TIME_STEP);
        }
        // 2. 越界与碰撞检测
        checkEntityOutOfBounds();
        // 3. 实体生成维持
        GameConfigBase* config = m_gameData->getConfig();
        if (getSpawnEntityCount() < config->getMaxEntityCount()) {
            spawnEntity();
        }
        // 4. 【新增】执行子类的自定义物理逻辑（如玩家飞机的移动）
        updateCustomPhysics(FIXED_TIME_STEP);
        // 消耗掉一个步长的时间
        m_accumulator -= FIXED_TIME_STEP;
    }

    // 【渲染层解耦】物理层彻底更新完毕后，再执行一次视图同步渲染
    updateViewEntities();

    if (m_gameView) {
        // 渲染动画（如背景滚动）依然使用真实的 frameTime 保证视觉上的丝滑
        m_gameView->updateAnimations(frameTime);
    }
}

void GameControllerBase::handleKeyPress(QKeyEvent* event)
{
    if (m_gameData->getGameState() != GameDataBase::Playing) return;

    char key = InputUtils::getUpperLetterFromKeyEvent(event);
    if (key == 0) return;

    GameEntity* targetEntity = m_gameData->findEntityByLetter(key);

    if (targetEntity) {
        targetEntity->setSelected(true);
        m_gameData->getConfig()->addSuccessCount();
        checkPassCondition();

        if (m_gameView) {
            playHitSound();
            targetEntity->startSelectedAnimation();

            connect(targetEntity, &GameEntity::selectedAnimationFinished, this, [this, targetEntity]() {
                if (m_gameView) {
                    removeEntityFromView(targetEntity);
                }
                m_gameData->removeEntity(targetEntity);
                });
        }
        else {
            m_gameData->removeEntity(targetEntity);
        }
    }
}

void GameControllerBase::spawnEntity()
{
    GameConfigBase* config = m_gameData->getConfig();
    QSet<QChar> existLetters = m_gameData->getExistLetters();
    QList<QChar> availableLetters;

    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        QChar candidate(letter);
        if (!existLetters.contains(candidate)) {
            availableLetters.append(candidate);
        }
    }

	if (availableLetters.isEmpty()) return; // 理论上不应该发生，因为关卡配置会限制最大实体数不超过26

    QChar newLetter = availableLetters.at(QRandomGenerator::global()->bounded(availableLetters.size()));

	GameEntity* entity = createNewEntity(); // 由子类实现创建具体实体的逻辑
    if (!entity) return;

    entity->setLetter(newLetter);

    int maxX = m_gameData->getSceneWidth() - getEntityWidth();
    int randomX = QRandomGenerator::global()->bounded(0, maxX + 1);

    entity->setPos(randomX, -getEntityHeight());
    entity->setFallSpeed(config->getFallSpeed());
    entity->setBoundingRect(QRectF(0, 0, getEntityWidth(), getEntityHeight()));

    if (m_gameView) {
        entity->startFadeIn(300);
    }

    m_gameData->addEntity(entity);
}

void GameControllerBase::checkEntityOutOfBounds()
{
    GameConfigBase* config = m_gameData->getConfig();
    qreal failLineY = m_gameData->getFailLineY();
    QList<GameEntity*> aliveEntities = m_gameData->getAliveEntities();

    for (auto entity : aliveEntities) {
        if (entity->isSelected()) continue;

        qreal entityBottomY = entity->y() + getEntityHeight();
        if (entityBottomY >= failLineY) {

            entity->setAlive(false);

            if (m_gameView) {
                playMissSound();
                removeEntityFromView(entity);
                m_gameData->removeEntityWithoutDestroy(entity);
            }
            else {
                m_gameData->removeEntity(entity);
            }
            config->addFailCount();
        }
    }
}

void GameControllerBase::checkPassCondition()
{
    GameConfigBase* config = m_gameData->getConfig();
    if (config->getSuccessCount() >= config->getPassTarget()) {
        int currentLevel = config->getLevel();
        if (currentLevel >= GameCommon::MAX_LEVEL) {
            m_gameData->setGameState(GameDataBase::GameOver);
            m_gameLoopTimer->stop();
        }
        else {
            m_gameData->setGameState(GameDataBase::LevelComplete);
            m_gameLoopTimer->stop();
        }
    }
}

void GameControllerBase::clearAllEntities()
{
    QList<GameEntity*> aliveEntities = m_gameData->getAliveEntities();
    for (auto entity : aliveEntities) {
        if (m_gameView) removeEntityFromView(entity);
        m_gameData->removeEntity(entity);
    }
    m_gameData->clearExistLetters();
    if (m_gameView) clearSuccessIconsFromView();
}

int GameControllerBase::getSpawnEntityCount() const
{
    return m_gameData ? m_gameData->getExistLetters().size() : 0;
}

void GameControllerBase::onNextLevelClicked()
{
    GameConfigBase* config = m_gameData->getConfig();
    int newLevel = config->getLevel() + 1;
    config->setLevel(newLevel);
    config->resetStats();
    clearAllEntities();

    m_gameData->setGameState(GameDataBase::Playing);
    m_gameLoopTimer->start();
}

void GameControllerBase::onRestartGameClicked()
{
    // 1. 停止当前游戏循环，清理场景通用实体
    stopGame();

    // 2. 重置基类统计数据（分数、成功率等）
    if (m_gameData && m_gameData->getConfig()) {
        m_gameData->getConfig()->resetStats();
        m_gameData->getConfig()->setLevel(1); // 默认重置回第1关
    }

    // 3. 【核心修复】：移除原有的 startGame() 调用！
    // 仅仅将状态退回 Idle，把控制权交还给 UI 层的“开始”按钮
    if (m_gameData) {
        m_gameData->setGameState(GameDataBase::Idle);
    }
}

void GameControllerBase::onExitGameClicked()
{
    emit exitGame();
}

void GameControllerBase::onLevelChanged(int level)
{
    float newSpeed = m_gameData->getConfig()->getFallSpeed();
    QList<GameEntity*> aliveEntities = m_gameData->getAliveEntities();
    for (auto entity : aliveEntities) {
        entity->setFallSpeed(newSpeed); // 更新下落速度
    }
}
