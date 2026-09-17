#include "planegamecontroller.h"
#include "playerentity.h"
#include "entityfactory.h"
#include "audiomanager.h"
#include <QRandomGenerator> // 新增：用于生成随机轨迹与初始速度
#include <QVariant>         // 新增：用于动态属性标记
#include "backpackdialog.h" // 引入背包界面头文件
#include "inpututils.h" // 确保引入输入工具
#include "mathutils.h"  // 确保引入数学工具
#include <memory> // 新增：智能指针支持
#include <QAbstractAnimation>


PlaneGameController::PlaneGameController(PlaneGameData* data, PlaneGameView* view, QObject* parent)
    : GameControllerBase(data, view, parent)
{
    initConnection(data, view);
}

void PlaneGameController::initConnection(PlaneGameData* data, PlaneGameView* view)
{
    if (view) {
        connect(view, &PlaneGameView::keyPressed, this, &GameControllerBase::handleKeyPress);
        connect(view, &PlaneGameView::nextLevelClicked, this, &GameControllerBase::onNextLevelClicked);
        connect(view, &PlaneGameView::restartGameClicked, this, &GameControllerBase::onRestartGameClicked);
        connect(view, &PlaneGameView::exitGameClicked, this, &GameControllerBase::onExitGameClicked);
    }
}

// 1. 重写按键处理：命中时不销毁，只标记并生成子弹
void PlaneGameController::handleKeyPress(QKeyEvent* event)
{
    if (m_gameData->getGameState() != GameDataBase::Playing) return;

    char key = InputUtils::getUpperLetterFromKeyEvent(event);
    if (key == 0) return;

    // ================== 【新增】优先匹配奖励单词 ==================
    if (m_rewardEntity && m_rewardEntity->isAlive() && !m_rewardEntity->isSelected()) {
        if (m_rewardEntity->checkNextLetter(QChar(key))) {

            // 判断是否已拼写完成
            if (m_rewardEntity->isCompleted()) {
                m_rewardEntity->setSelected(true); // 锁定防止重复操作

                // 给玩家恢复生命值
                PlayerEntity* player = getGameView()->getPlayer();
                if (player) {
                    player->heal(1.0f); // 更改为 float

                    AudioManager::instance()->playEffect(AUDIO_EFFECT_HEAL); // 播放回血音效
                }

                // 触发特效销毁
                m_rewardEntity->startFadeOut(300);
                connect(m_rewardEntity, &GameEntity::fadeOutFinished, this, [this]() {
                    if (m_rewardEntity) {
                        getGameView()->removeItem(m_rewardEntity);
                        getGameData()->removeEntity(m_rewardEntity);
                        m_rewardEntity = nullptr;
                    }
					}, Qt::UniqueConnection);// 确保连接只触发一次
            }
            return; // 成功匹配奖励单词中的字母，拦截本次按键，不再向下匹配敌机
        }
    }

    // ================== 【新增：优先匹配 Boss 身上暴露的破甲部位】 ==================
    BossEntity* boss = getGameData()->getActiveBoss();
    if (boss && boss->isAlive()) {
        QList<BossPart>& parts = boss->getParts();
        for (int i = 0; i < parts.size(); ++i) {
            BossPart& part = parts[i];

            // 如果部位已破甲、未被摧毁，且字母匹配玩家按键
            if (part.isExposed && !part.isDestroyed && part.letter == key) {
                // 1. 立即清空该字母，防止被玩家连按重复发射多枚飞弹
                part.letter = QChar();
                getGameData()->removeExistLetter(QChar(key)); // 从全局字母池释放

                // ======== 新增：触发高能锁定特效 ========
                part.isLocked = true;
				part.lockTime = QDateTime::currentMSecsSinceEpoch(); // 记录锁定开始时间，后续可以在 BossPart 的 updateLogic 中根据时间控制特效持续和飞弹发射的同步

                // 2. 生成追踪飞弹
                BulletEntity* bullet = EntityFactory::instance()->createEntity<BulletEntity>();
                PlayerEntity* player = getGameView()->getPlayer();
                QPointF playerCenter = player->scenePos() + QPointF(PlayerEntity::PLAYER_WIDTH / 2.0, PlayerEntity::PLAYER_HEIGHT / 2.0);

                bullet->setSpawnPosition(playerCenter, m_spawnLeftWing);
                m_spawnLeftWing = !m_spawnLeftWing;

                // 3. 锁定该 Boss 部位
                bullet->setBossTarget(boss, &part);

                m_activeBullets.append(bullet);
                getGameView()->addBulletToScene(bullet);

                AudioManager::instance()->playEffect(AUDIO_EFFECT_MISSILE);
                return; // 成功拦截，直接返回
            }
        }
    }
    // ===========================================================================

    //  PlaneEntity (普通敌机) 匹配逻辑
    PlaneEntity* targetPlane = qobject_cast<PlaneEntity*>(m_gameData->findEntityByLetter(key));

    if (targetPlane) {
        // 将目标标记为选中，避免被其他子弹重复锁定
        targetPlane->setSelected(true);

        // 实例化子弹并交由内存池管理
        BulletEntity* bullet = EntityFactory::instance()->createEntity<BulletEntity>();

        // 获取玩家中心坐标
        PlayerEntity* player = getGameView()->getPlayer();
        QPointF playerCenter = player->scenePos() + QPointF(PlayerEntity::PLAYER_WIDTH / 2.0, PlayerEntity::PLAYER_HEIGHT / 2.0);

        // 设置子弹并加入视图
        bullet->setSpawnPosition(playerCenter, m_spawnLeftWing);
        m_spawnLeftWing = !m_spawnLeftWing; // 左右交替
        bullet->setTarget(targetPlane);

        m_activeBullets.append(bullet);
        getGameView()->addBulletToScene(bullet);

        // 播放发射音效（复用打击音效或你配置的其他音效）
        AudioManager::instance()->playEffect(AUDIO_EFFECT_MISSILE);
    }
}

void PlaneGameController::stopGame()
{
    // 清理所有残留追踪子弹
    for (BulletEntity* bullet : m_activeBullets) {
        if (getGameView()) getGameView()->removeBulletFromScene(bullet);
        bullet->destroy();
    }
    m_activeBullets.clear();

    // 清理所有残留机炮子弹
    for (MachineGunBulletEntity* bullet : m_activeMachineGunBullets) {
        if (getGameView()) getGameView()->removeMachineGunBulletFromScene(bullet);
        bullet->destroy();
    }
    m_activeMachineGunBullets.clear();

    // 清理所有残留Boss子弹
    for (EnemyBulletEntity* bullet : m_enemyBullets) {
        if (getGameView() && bullet->scene()) {
            getGameView()->removeItem(bullet);
        }
        bullet->destroy();
    }
    m_enemyBullets.clear();

	cleanupBoss(); // 清理 Boss 实体和动画，防止残留

    // ================== 【新增：游戏结束时清理奖励单词】 ==================
    m_rewardEntity = nullptr;
    m_isGeneratingWord = false;
    m_rewardTimer = 0.0f;
    // ===================================================================

    GameControllerBase::stopGame();
}

void PlaneGameController::startGame()
{
    m_bossSpawnedThisLevel = false;
    m_isBossPhase = false;
    m_bossWarningTimer = 0.0f;
    cleanupBoss();

    // 【新增】：每次真正重新开始游戏时（非暂停恢复），才重置总分数
    if (getGameData()->getGameState() == GameDataBase::Idle || getGameData()->getGameState() == GameDataBase::GameOver) {
        PlaneGameConfig::instance()->resetScore();
    }

    // 1. 每次真正开始游戏时，重置玩家血量为满血 (配置中默认为 18 点)
    PlayerEntity* player = getGameView()->getPlayer();
    if (player) {
        player->initHealth(player->attributes()->maxHp());
    }

    // 2. 调用基类的开始逻辑（触发定时器和状态切换）
    GameControllerBase::startGame();
}

void PlaneGameController::onNextLevelClicked()
{
    m_bossSpawnedThisLevel = false;
    m_isBossPhase = false;
    m_bossWarningTimer = 0.0f;
	cleanupBoss(); // 清理 Boss 实体和动画，防止残留

    // 1. 在进入下一关前，安全销毁当前关卡所有残留的追踪子弹
    for (BulletEntity* bullet : m_activeBullets) {
        if (getGameView() && bullet->scene()) {
            getGameView()->removeItem(bullet);
        }
        bullet->destroy();
    }
    m_activeBullets.clear();

    // 清理所有残留机炮子弹
    for (MachineGunBulletEntity* bullet : m_activeMachineGunBullets) {
        if (getGameView()) getGameView()->removeMachineGunBulletFromScene(bullet);
        bullet->destroy();
    }
    m_activeMachineGunBullets.clear();

    // 清理所有残留Boss子弹
    for (EnemyBulletEntity* bullet : m_enemyBullets) {
        if (getGameView()) getGameView()->removeItem(bullet);
        bullet->destroy();
    }
    m_enemyBullets.clear();

    // ================== 【新增：彻底清理奖励单词状态】 ==================
    // 注意：基类的 onNextLevelClicked 会调用 clearAllEntities -> removeEntityFromView 安全剥离图元
    // 我们只需要在这里置空指针，重置请求和定时器状态，避免跨关卡出错
    m_rewardEntity = nullptr;
    m_isGeneratingWord = false;
    m_rewardTimer = 0.0f;
    // ===================================================================

    // 2. 调用基类的逻辑（清理敌机实体、增加关卡等级、重启定时器等）
    GameControllerBase::onNextLevelClicked();

}

void PlaneGameController::cleanupBoss()
{
	PlaneGameData* data = getGameData(); // 防御性编程：Data 可能意外丢失（如游戏重置时未及时剥离指针），或者当前没有 Boss 实体
    if (!data) return;

    BossEntity* boss = data->getActiveBoss(); 
    data->setActiveBoss(nullptr); 
    if (!boss) return;

    releaseBossLetters(boss); 

    // 停止 Boss 相关的所有动画，防止它们在实体销毁后继续尝试访问已销毁的对象
	const QList<QAbstractAnimation*> animations = boss->findChildren<QAbstractAnimation*>(); 
    for (QAbstractAnimation* animation : animations) {
        if (animation) {
            animation->stop();
        }
    }

    boss->disconnect();
    if (boss->scene()) {
		boss->scene()->removeItem(boss);// 从场景中剥离图元，防止残留图元导致的视觉错误和内存泄漏
    }
    boss->destroy();
}

void PlaneGameController::releaseBossLetters(BossEntity* boss)
{
    if (!boss || !getGameData()) return;

    QList<BossPart>& parts = boss->getParts();
    for (BossPart& part : parts) {
        if (!part.letter.isNull()) {
            getGameData()->removeExistLetter(part.letter);
            part.letter = QChar();
        }
    }
}


// ---------------------------------------------------------
// 具体业务实现
// ---------------------------------------------------------

GameEntity* PlaneGameController::createNewEntity() {
    // Boss 阶段暂停普通敌机生成
    if (m_isBossPhase) {
        return nullptr;
    }

    PlaneEntity* plane = EntityFactory::instance()->createEntity<PlaneEntity>(); 

    // 【核心新增】：动态阶段与类型分配
    int currentLevel = m_gameData->getConfig()->getLevel();
    int currentStage = 0;

    // 关卡映射到阶段：3、5、2 (1-3, 4-8, 9-10)
    if (currentLevel >= 1 && currentLevel <= 3) {
        currentStage = 2; // 敌占区
    }
    else if (currentLevel >= 4 && currentLevel <= 8) {
        currentStage = 1; // 虫巢
    }
    else {
        currentStage = 0; // 外星母舰
    }

    // 随机当前阶段的 4 种类型之一
    int enemyType = QRandomGenerator::global()->bounded(4);

    // 注入配置
    plane->setStageAndType(currentStage, enemyType);

    // 随机分配三种运动轨迹（0: 折线反弹, 1: 正弦波S型, 2: 追踪玩家）
	int randTrajectory = QRandomGenerator::global()->bounded(3); // 生成0、1或2
    plane->setTrajectoryType(static_cast<PlaneEntity::TrajectoryType>(randTrajectory));

    // 赋予初始水平速度动量。使其一开始就有偏向，避免纯垂直下落
    int sign = QRandomGenerator::global()->bounded(2) == 0 ? 1 : -1;
    qreal initialVx = sign * (QRandomGenerator::global()->bounded(40) + 40.0); // -80~-40 或 40~80 之间
    qreal vy = PlaneGameConfig::instance()->getFallSpeed();

    plane->setVelocity(initialVx, vy);

    return plane;

}

void PlaneGameController::updateViewEntities() {
    if (getGameView()) {
        // 只负责通知视图层刷新坏飞机的绘制，不包含任何逻辑状态修改
        getGameView()->updateAllPlanes();
    }
}

void PlaneGameController::removeEntityFromView(GameEntity* entity) {
    if (!getGameView()) return;
    // 【核心修复】：使用安全的类型转换区分敌机和奖励单词
    PlaneEntity* plane = qobject_cast<PlaneEntity*>(entity);
    if (plane) {
        getGameView()->removePlaneFromScene(plane);
        return;
    }
    RewardWordEntity* reward = qobject_cast<RewardWordEntity*>(entity);
    if (reward) {
        getGameView()->removeItem(reward); // 奖励单词直接从场景剔除，不走敌机的爆炸逻辑
        return;
    }
}

void PlaneGameController::clearSuccessIconsFromView() {
	// 目前太空大战没有成功计数小图标，这个方法可以留空或者实现为清理特效图层的残留特效图元
}

void PlaneGameController::playHitSound() {
	// 可以根据不同的击中类型（如普通击中、暴击等）播放不同的音效，这里暂时统一使用一个音效
}

void PlaneGameController::playMissSound() {
	// 太空大战没有明显的丢失反馈音效，如果需要可以在这里播放一个警告音效或者干脆不播放任何声音
}

int PlaneGameController::getEntityWidth() const {
    return PlaneEntity::PLANE_WIDTH;
}

int PlaneGameController::getEntityHeight() const {
    return PlaneEntity::PLANE_HEIGHT;
}

int PlaneGameController::getSpawnEntityCount() const
{
    const PlaneGameData* data = static_cast<const PlaneGameData*>(m_gameData);
    return data ? data->getAlivePlanes().size() : 0;
}

void PlaneGameController::updateCustomPhysics(float deltaTime)
{
	if (!getGameView() || !getGameData()) return; 

    // 1. 玩家自身逻辑更新 (移动与射击冷却)
    updatePlayerPhysicsAndShooting(deltaTime);

    // 2. 驱动敌机复杂轨迹计算与碰撞检测
    updateEnemiesAndPlayerCollision(deltaTime);

    // 3. 驱动追踪飞弹逻辑与碰撞检测
    updateGuidedBullets(deltaTime);

    // 4. 驱动机炮子弹逻辑与双轨碰撞检测
    updateMachineGunBullets(deltaTime);

    // 5. 奖励模式定时生成逻辑
    updateRewardWords(deltaTime);
    
	// 6. 驱动Boss子弹的移动与碰撞检测
    updateEnemyBulletPhysics(deltaTime);

    // 7. Boss战专用物理更新方法，处理Boss的特殊移动模式和攻击逻辑
    updateBossPhysics(deltaTime);
}

void PlaneGameController::checkPassCondition()
{
    GameConfigBase* config = m_gameData->getConfig();
    int currentLevel = config->getLevel();
    // 判断是否为 Boss 关
    bool isBossLevel = (currentLevel == 2 || currentLevel == 7 || currentLevel >= GameCommon::MAX_LEVEL);

    if (config->getSuccessCount() >= config->getPassTarget()) {

        // 【核心新增】：如果是 Boss 关卡
        if (isBossLevel) {
            // 如果还未进入 Boss 阶段，则触发警告并拦截过关
            if (!m_isBossPhase && !m_bossSpawnedThisLevel) {
                m_isBossPhase = true;
                m_bossWarningTimer = 0.0f;
                if (getGameView()) {
					getGameView()->playBossWarningAnimation();  // 播放 Boss 警告动画，提示玩家即将进入 Boss 战
                }
                return; // 拦截过关逻辑！
            }
            // 如果已经在 Boss 阶段（说明 Boss 还没死，但小怪分数已经溢出），继续拦截
            if (m_isBossPhase) {
                return;
            }
        }

        // 1. 立即停止物理循环，让战场上的子弹和飞机悬停冻结
        m_gameLoopTimer->stop();

        // 2. 调用视图层播放重打击感过关特效（无论是否为最后一关都播放）
        if (getGameView()) {
            getGameView()->playMissionSuccessAnimation();
            // 播放振奋的升级/通关音效
            AudioManager::instance()->playEffect(AUDIO_EFFECT_LEVEL_UP);
        }

        // 3. 利用 QTimer::singleShot 延迟 1.8 秒，等待特效动画平滑播放完毕
        QTimer::singleShot(1800, this, [this, currentLevel]() {
            // 严谨起见，防止玩家在这 1.8 秒内强退游戏导致指针异常
            if (m_gameData && m_gameData->getGameState() == GameDataBase::Playing) {

                if (currentLevel >= GameCommon::MAX_LEVEL) {
                    // 如果是最终通关，进入 GameOver（或者您可以后续自定义一个 GameClear 通关结算状态）
                    m_gameData->setGameState(GameDataBase::GameOver);
                }
                else {
                    // 如果不是最后一关，进入正常的关卡结算界面
                    m_gameData->setGameState(GameDataBase::LevelComplete);
                }
            }
            });
    }
}

void PlaneGameController::updateEnemyBulletPhysics(float deltaTime)
{
    PlayerEntity* player = getGameView()->getPlayer();
    qreal sceneHeight = getGameData()->getSceneHeight();
    qreal sceneWidth = getGameData()->getSceneWidth();

    for (int i = m_enemyBullets.size() - 1; i >= 0; --i) {
        EnemyBulletEntity* bullet = m_enemyBullets[i];
        bullet->updateLogic(deltaTime);

        // 1. 越界检查
        if (bullet->y() > sceneHeight + 100 || bullet->y() < -200 ||
            bullet->x() < -100 || bullet->x() > sceneWidth + 100) {
            // 【安全校验】：确保子弹在场景中才移除
            if (getGameView() && bullet->scene()) {
                getGameView()->removeItem(bullet);
            }
            bullet->destroy();
            m_enemyBullets.removeAt(i);
            continue;
        }

        // 2. 玩家碰撞检测
        if (player && player->isAlive() && !player->isInvincible()) {
            if (bullet->collidesWithItem(player)) {
                // 【核心修复】：必须先剥离子弹并回收，再触发可能改变游戏状态的扣血逻辑！
                float damageDealt = bullet->getDamage();
                if (getGameView() && bullet->scene()) {
                    getGameView()->removeItem(bullet);
                }
                bullet->destroy();
                m_enemyBullets.removeAt(i);

                // 玩家受到伤害
                bool isDestroyed = player->takeDamage(bullet->getDamage());

                // 播放反馈
                if (isDestroyed) {
                    player->startSelectedAnimation(); // 死亡爆炸动画
                    AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION);
                    getGameData()->setGameState(GameDataBase::GameOver);
                }
                else {
                    player->startShieldAnimation(); // 护盾受击闪烁反馈
                    AudioManager::instance()->playEffect(AUDIO_EFFECT_PLAYER_HIT);
                }

                continue; // 处理完毕，跳入下一发子弹的检测
            }
        }
    }


}

void PlaneGameController::onBossFireBullet(int type, const QPointF& pos, const QPointF& vel, float dmg)
{
    // 利用对象池创建敌方子弹
    EnemyBulletEntity* bullet = EntityFactory::instance()->createEntity<EnemyBulletEntity>();
    bullet->initBullet(type, pos, vel, dmg);

    m_enemyBullets.append(bullet);
    if (getGameView()) {
        getGameView()->addItem(bullet);
    }
}

void PlaneGameController::onBossFireTargetedBullet(int type, const QPointF& pos, float speed, float dmg)
{
    PlayerEntity* player = getGameView()->getPlayer();
    if (!player || !player->isAlive()) return;

    // 算法：计算从 Boss 发射点指向玩家中心的单位向量
    QPointF playerCenter = player->scenePos() + QPointF(PlayerEntity::PLAYER_WIDTH / 2.0, PlayerEntity::PLAYER_HEIGHT / 2.0);
    QPointF dir = playerCenter - pos;

    qreal dist = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (dist > 0.1) {
        dir /= dist;
    }
    else {
        dir = QPointF(0, 1); // 兜底向下发射
    }

    QPointF velocity = dir * speed;
    onBossFireBullet(type, pos, velocity, dmg);
}

void PlaneGameController::updatePlayerPhysicsAndShooting(float deltaTime)
{
	PlayerEntity* player = getGameView()->getPlayer(); // 防御性编程：玩家可能意外丢失（如被销毁后未及时剥离指针），或者处于无敌状态（暂时不允许移动）
    if (!player || !player->isAlive()) return;

    bool goLeft = getGameView()->isLeftPressed();
    bool goRight = getGameView()->isRightPressed();
    // 【新增】：获取上下按键的状态
    bool goUp = getGameView()->isUpPressed();
    bool goDown = getGameView()->isDownPressed();

    // ================== 【核心接入 1：使用属性组件控制机动速度】 ==================
    // 之前硬编码为 8.0。现在挂钩 deltaTime，确保帧率波动时速度平滑一致
    qreal moveSpeed = player->attributes()->moveSpeed() * deltaTime;

    qreal currentX = player->x();
    qreal currentY = player->y(); // 【新增】获取当前 Y 坐标
    int direction = 0;

    if (goLeft && !goRight) {
        currentX -= moveSpeed;
        direction = -1;
    }
    else if (goRight && !goLeft) {
        currentX += moveSpeed;
        direction = 1;
    }
    // 【新增】：处理垂直移动
    if (goUp && !goDown) {
        currentY -= moveSpeed;
    }
    else if (goDown && !goUp) {
        currentY += moveSpeed;
    }

    // 边界限制计算
    qreal maxX = getGameData()->getSceneWidth() - PlayerEntity::PLAYER_WIDTH;
    qreal maxY = getGameData()->getSceneHeight() - PlayerEntity::PLAYER_HEIGHT; // 【新增】下边界

    // 限制 X 轴不出界
    if (currentX < 0) currentX = 0;
    if (currentX > maxX) currentX = maxX;

    // 【新增】：限制 Y 轴不出界
    if (currentY < 0) currentY = 0;   // 防止飞出屏幕顶部
    if (currentY > maxY) currentY = maxY; // 防止飞出屏幕底部

    // 【修改】：使用 setPos 同时更新 X 和 Y 坐标
    player->setPos(currentX, currentY);
    player->setMovingDirection(direction);

    // 使用物理步长精确驱动战机的动画过渡
    player->updateLogic(deltaTime);

    // ================== 【机炮连发逻辑】 ==================
    if (getGameView()->isSpacePressed()) {
        m_fireCooldown -= deltaTime;
        if (m_fireCooldown <= 0.0f) {
            // 冷却完毕，生成子弹
            MachineGunBulletEntity* mgBullet = EntityFactory::instance()->createEntity<MachineGunBulletEntity>();
            QPointF playerCenter = player->scenePos() + QPointF(PlayerEntity::PLAYER_WIDTH / 2.0, PlayerEntity::PLAYER_HEIGHT / 2.0);

            bool isCrit = false;
            float baseDamage = player->attributes()->machineGunDamage();
            float finalDamage = player->attributes()->calculateOutputDamage(baseDamage, isCrit);
			mgBullet->setDamage(finalDamage); // 将最终伤害值传递给子弹实体，供其碰撞时使用

            mgBullet->setSpawnPosition(playerCenter);
            m_activeMachineGunBullets.append(mgBullet);
            getGameView()->addMachineGunBulletToScene(mgBullet);

            // 播放机炮开火音效
            AudioManager::instance()->playEffect(AUDIO_EFFECT_MACHINE_GUN);

            // 重置冷却时间
            m_fireCooldown = FIRE_RATE;
        }
    }
    else {
        // 松开空格键时将冷却归零，保证下一次按下一瞬间能立刻开火，手感最好
        m_fireCooldown = 0.0f;
    }
}

void PlaneGameController::updateEnemiesAndPlayerCollision(float deltaTime)
{
    PlayerEntity* player = getGameView()->getPlayer();
    QPointF playerPos(0, 0);
    if (player) {
        playerPos = player->pos();
    }

    qreal screenWidth = getGameData()->getSceneWidth();
    QList<PlaneEntity*> planes = getGameData()->getAlivePlanes();

    for (PlaneEntity* plane : planes) {
        if (!plane) continue;

        // 延迟初始化初始坐标
        if (!plane->property("isStartPosInited").toBool()) {
            plane->setStartPosition(plane->pos());
            plane->setProperty("isStartPosInited", true);
        }

        // 调用核心计算引擎，驱动其水平、垂直位移和姿态帧切换
        plane->updateTrajectory(deltaTime, screenWidth, playerPos);

        // ================== 【玩家与敌机的物理碰撞检测】 ==================
        // 防御性判断：玩家必须存活，且敌机尚未被玩家的子弹锁定
        if (player && player->isAlive() && !plane->property("hitFrozen").toBool() && !plane->isSelected()) {
            if (plane->collidesWithItem(player)) {

                // 【无敌帧分支】
                if (player->isInvincible()) {
                    if (getGameView()) {
                        getGameView()->removePlaneFromScene(plane);
                    }
                    m_gameData->removeEntity(plane);
                    continue;
                }

                // 玩家受到碰撞伤害（撞击一次扣除 1 点生命值）
                bool isPlayerDestroyed = player->takeDamage(1.0f); // 更改为 float

                // 根据存活状态播放对应动画
                if (isPlayerDestroyed) {
                    player->startSelectedAnimation();
                    AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION);
                }
                else {
                    player->startShieldAnimation();
                    AudioManager::instance()->playEffect(AUDIO_EFFECT_PLAYER_HIT);
                }

                // 直接爆炸销毁敌机
                if (getGameView()) {
                    getGameView()->removePlaneFromScene(plane);
                }
                m_gameData->removeEntity(plane);

                // 检查玩家是否阵亡
                if (isPlayerDestroyed) {
                    getGameData()->setGameState(GameDataBase::GameOver);
                }
            }
        }
    }
}

void PlaneGameController::updateGuidedBullets(float deltaTime)
{
    PlayerEntity* player = getGameView()->getPlayer();

    for (int i = m_activeBullets.size() - 1; i >= 0; --i) {
        BulletEntity* bullet = m_activeBullets[i];
        bullet->updateLogic(deltaTime);

        PlaneEntity* target = bullet->getTarget();  // 追踪普通飞机
		BossEntity* bossTarget = bullet->getBossTarget(); // 追踪Boss
		BossPart* bossPart = bullet->getBossPartTarget(); // 追踪飞弹锁定Boss的具体部位

        // ================== 【新增：处理追踪 Boss 部位的飞弹】 ==================
        if (bossTarget && bossPart) {
            if (!bossTarget->isAlive()) {
                getGameView()->removeBulletFromScene(bullet);
                bullet->destroy();
                m_activeBullets.removeAt(i);
                continue;
            }

            QPointF bulletCenter = bullet->scenePos() + QPointF(BulletEntity::BULLET_WIDTH / 2.0, BulletEntity::BULLET_HEIGHT / 2.0);
            QPointF partCenter = bossTarget->scenePos() + QPointF(BossEntity::BOSS_WIDTH / 2.0, BossEntity::BOSS_HEIGHT / 2.0) + bossPart->localRect.center();

            float dist = MathUtils::getDistance(bulletCenter, partCenter);

            // 距离小于 40 像素视为精准命中该部位
            if (dist < 40.0f) {
                // 1. 彻底摧毁该部位！
                bossPart->isDestroyed = true;
                bossPart->isExposed = false; // 移除破甲状态
                bossPart->isLocked = false; // ======== 新增：飞弹命中，解除锁定 ========

                // 2. 播放强烈的爆炸音效
                if (bossTarget->getBossType() == 1) AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ZERG);
                else if (bossTarget->getBossType() == 0) AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION);
                else AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ALIEN);

                // 3. 给予玩家大量分数奖励
                PlaneGameConfig::instance()->addScore(1000);

                // 4. 在该局部坐标触发一个巨大的爆炸火花，并传入具体部位以实现局部闪白
                QPointF localHitPos = partCenter - bossTarget->scenePos() - QPointF(BossEntity::BOSS_WIDTH / 2.0, BossEntity::BOSS_HEIGHT / 2.0);
                bossTarget->startHitAnimation(localHitPos, bossPart);
				bossTarget->playPartExplosion(bossPart); // 根据部位信息选择对应的爆炸特效图元进行播放

                // 5. 销毁子弹
                getGameView()->removeBulletFromScene(bullet);
                bullet->destroy();
                m_activeBullets.removeAt(i);

                // 6. 检查是否所有部位都已摧毁？
                if (bossTarget->areAllPartsDestroyed() && !bossTarget->isDying()) {

                    // 1. 启动 Boss 的 3 秒原地连环爆炸序列
                    bossTarget->startDefeatedSequence();

                    // 2. 监听并播放连环爆炸过程中的音效
                    connect(bossTarget, &BossEntity::chainExplosionTriggered, this, [bossTarget]() {
                        if (bossTarget->getBossType() == 1) AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ZERG);
                        else if (bossTarget->getBossType() == 0) AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION);
                        else AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ALIEN);
                        }, Qt::UniqueConnection);

                    // 3. 监听 3 秒结束后的最终死亡结算
                    connect(bossTarget, &BossEntity::deathSequenceFinished, this, [this, bossTarget]() {
                        // 播放我们在第一步写好的，极其震撼的巨型能量坍缩大爆炸
                        bossTarget->startSelectedAnimation();

                        // 【核心】：解除 Boss 阶段锁定，强制触发通关！
                        m_isBossPhase = false;
                        checkPassCondition();

                        // 最终毁灭动画播放完毕后，彻底销毁 Boss 实体释放内存
                        connect(bossTarget, &GameEntity::selectedAnimationFinished, this, [this, bossTarget]() {
                            if (getGameView() && bossTarget->scene() == getGameView()) {
                                getGameView()->removeItem(bossTarget);
                            }

                            releaseBossLetters(bossTarget); 

                            if (getGameData() && getGameData()->getActiveBoss() == bossTarget) {
                                getGameData()->setActiveBoss(nullptr);
                            }
                            bossTarget->destroy();
                            }, Qt::UniqueConnection);

                        }, Qt::UniqueConnection);
                }
            }
            continue; // 跳过下方的普通敌机检测
        }
        // =========================================================================

        // 防御性编程：目标意外丢失直接自毁
        if (!target || !getGameData()->getAlivePlanes().contains(target)) {
            getGameView()->removeBulletFromScene(bullet);
            bullet->destroy();
            m_activeBullets.removeAt(i);
            continue;
        }

        QPointF bulletCenter = bullet->scenePos() + QPointF(BulletEntity::BULLET_WIDTH / 2.0, BulletEntity::BULLET_HEIGHT / 2.0);
        QPointF targetCenter = target->scenePos() + QPointF(PlaneEntity::PLANE_WIDTH / 2.0, PlaneEntity::PLANE_HEIGHT / 2.0);

        float dist = MathUtils::getDistance(bulletCenter, targetCenter);

        // 距离小于 40 像素视为命中装甲
        if (dist < 40.0f) {
            bool destroyed = target->takeDamage(9999, player);

            getGameData()->getConfig()->addSuccessCount();
            checkPassCondition();

            if (target->getStage() == 1) {
                AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ZERG);
            }
            else if (target->getStage() == 0) {
                AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ALIEN);
            }
            else {
                AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION);
            }

            target->setProperty("hitFrozen", true);
            target->startSelectedAnimation();

            connect(target, &GameEntity::selectedAnimationFinished, this, [this, target]() {
                if (m_gameData && m_gameData->getAliveEntities().contains(target)) {
                    if (getGameView()) {
                        getGameView()->removePlaneFromScene(target);
                    }
                    m_gameData->removeEntity(target);
                }
                }, Qt::UniqueConnection);

            if (getGameView()) {
                getGameView()->removeBulletFromScene(bullet);
            }
            bullet->destroy();
            m_activeBullets.removeAt(i);
        }
    }
}

void PlaneGameController::updateMachineGunBullets(float deltaTime)
{
    PlayerEntity* player = getGameView()->getPlayer();
    BossEntity* boss = getGameData()->getActiveBoss(); // 获取当前 Boss

    for (int i = m_activeMachineGunBullets.size() - 1; i >= 0; --i) {
        MachineGunBulletEntity* bullet = m_activeMachineGunBullets[i];
        bullet->updateLogic(deltaTime);

        if (!bullet->isAlive()) {
            getGameView()->removeMachineGunBulletFromScene(bullet);
            bullet->destroy();
            m_activeMachineGunBullets.removeAt(i);
            continue;
        }

        bool hit = false;

        // 伤害在发射瞬间由玩家属性组件写入子弹，命中时只消费该发子弹自己的数值。
        float finalDamage = bullet->getDamage();

        // ================== 【修改：优先检测是否击中 Boss 多部位】 ==================
        // 增加 !boss->isDying() 判断，濒死连爆期间屏蔽机枪伤害
        if (boss && boss->isAlive() && !boss->isDying() && bullet->collidesWithItem(boss)) {
            // 此时子弹只是进入了 Boss 的大外框区域，还不能算作命中！

            // 1. 获取子弹尖端在世界坐标系的位置
            QPointF bulletTipScenePos = bullet->scenePos() + QPointF(MachineGunBulletEntity::BULLET_WIDTH / 2.0, 0);
            // 2. 将世界坐标映射到 Boss 自身的局部坐标系
            QPointF localHitPos = boss->mapFromScene(bulletTipScenePos);

            // 3. 使用新计算出的 finalDamage尝试对 Boss 的某个具体部位造成伤害
            BossPart* hitPart = boss->checkAndDamagePart(localHitPos, finalDamage);

            // 只有子弹真正接触到了我们设定的部位（如左右翼、核心），才视为有效命中并销毁子弹
            if (hitPart) {
                hit = true;  // 真正命中了部位才销毁子弹！
                
                if (hitPart->currentHp <= 0 && hitPart->letter.isNull()) {
                    // 破甲成功！从 A-Z 中随机分配一个当前场上不存在的字母
                    QChar newLetter;
                    do {
                        newLetter = QChar('A' + QRandomGenerator::global()->bounded(26));
                    } while (getGameData()->getExistLetters().contains(newLetter));

                    hitPart->letter = newLetter;
                    hitPart->isExposed = true; // 暴露出全息字母，等待玩家打字飞弹

                    // 将字母注册到全局数据层，避免后续生成的普通敌机字母冲突
                    getGameData()->addExistLetter(newLetter);

                    // 播放破甲的沉闷/爆裂音效，给玩家明显的反馈
                    AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION);
                    // ================== 【核心新增：破甲瞬间也播放局部爆炸动画】 ==================
                    boss->playPartExplosion(hitPart);
                    // =========================================================================
                }
                else if (!hitPart->isExposed) {
                    // 还在刮痧破甲阶段，播放普通击中音效
                    if (boss->getBossType() == 1) AudioManager::instance()->playEffect(AUDIO_EFFECT_HIT_ZERG);
                    else if (boss->getBossType() == 0) AudioManager::instance()->playEffect(AUDIO_EFFECT_HIT_ENEMY);
                    else AudioManager::instance()->playEffect(AUDIO_EFFECT_HIT_ALIEN);
                }

                // 只要击中了有效部位（无论是否破甲），都触发受击火花特效与局部闪白
                boss->startHitAnimation(localHitPos, hitPart);
            }
        }
        // =========================================================================
            
        // 下面保留原有的普通敌机碰撞检测逻辑
        if (!hit) {
            QList<PlaneEntity*> planes = getGameData()->getAlivePlanes();
            for (PlaneEntity* target : planes) {
                if (target->property("hitFrozen").toBool() || target->isSelected()) continue;

                if (bullet->collidesWithItem(target)) {
                    hit = true;
                    bool destroyed = target->takeDamage(finalDamage, player);

                    if (destroyed) {
                        PlaneGameConfig::instance()->addSuccessCount();
						checkPassCondition(); // 这里的过关检测会在 Boss 战中被 m_isBossPhase 拦截，确保玩家必须击破 Boss 才能过关

                        if (target->getStage() == 1) {
                            AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ZERG);
                        }
                        else if (target->getStage() == 0) {
                            AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION_ALIEN);
                        }
                        else {
                            AudioManager::instance()->playEffect(AUDIO_EFFECT_EXPLOSION);
                        }

                        target->setProperty("hitFrozen", true);
                        target->startSelectedAnimation();

                        connect(target, &GameEntity::selectedAnimationFinished, this, [this, target]() {
                            if (m_gameData && m_gameData->getAliveEntities().contains(target)) {
                                if (getGameView()) {
                                    getGameView()->removePlaneFromScene(target);
                                }
                                m_gameData->removeEntity(target);
                            }
                            }, Qt::UniqueConnection);
                    }
                    else {
                        target->startHitAnimation();
                        if (target->getStage() == 1) {
                            AudioManager::instance()->playEffect(AUDIO_EFFECT_HIT_ZERG);
                        }
                        else if (target->getStage() == 0) {
                            AudioManager::instance()->playEffect(AUDIO_EFFECT_HIT_ALIEN);
                        }
                        else {
                            AudioManager::instance()->playEffect(AUDIO_EFFECT_HIT_ENEMY);
                        }
                    }
                    break;
                }
            }
        }

        if (hit) {
            getGameView()->removeMachineGunBulletFromScene(bullet);
            bullet->destroy();
            m_activeMachineGunBullets.removeAt(i);
        }
    }
}

void PlaneGameController::updateRewardWords(float deltaTime)
{
    if (PlaneGameConfig::instance()->isRewardModeEnabled()) {
        m_rewardTimer += deltaTime;

        // 每 10 秒触发一次，并且当前场上没有奖励单词，且没有正在请求
        if (m_rewardTimer >= 10.0f && !m_rewardEntity && !m_isGeneratingWord) {
            m_isGeneratingWord = true;

            std::shared_ptr<QMetaObject::Connection> conn = std::make_shared<QMetaObject::Connection>();
            *conn = connect(WordGenerator::instance(), &WordGenerator::wordGenerated, this, [this, conn](const QString& word) {
                QObject::disconnect(*conn);

                m_isGeneratingWord = false;
                m_rewardTimer = 0.0f;

                m_rewardEntity = EntityFactory::instance()->createEntity<RewardWordEntity>();
                m_rewardEntity->setWord(word);

                qreal startY = 100.0 + QRandomGenerator::global()->bounded(100);
                m_rewardEntity->setPos(getGameData()->getSceneWidth(), startY);

                getGameData()->addEntity(m_rewardEntity);
                getGameView()->addItem(m_rewardEntity);
                m_rewardEntity->startFadeIn(300);
                });

            WordGenerator::instance()->requestWordAsync();
        }
    }
    else {
        m_rewardTimer = 0.0f;
    }

    if (m_rewardEntity && !m_rewardEntity->isAlive()) {
        getGameView()->removeItem(m_rewardEntity);
        getGameData()->removeEntity(m_rewardEntity);
        m_rewardEntity = nullptr;
    }
}

void PlaneGameController::updateBossPhysics(float deltaTime)
{
    int currentLevel = getGameData()->getConfig()->getLevel();
    bool isBossLevel = (currentLevel == 2 || currentLevel == 7 || currentLevel >= GameCommon::MAX_LEVEL);

    // 【修改】：只有当触发了 m_isBossPhase 拦截，才开始倒计时生成 Boss
    if (isBossLevel && m_isBossPhase && !m_bossSpawnedThisLevel) {
        m_bossWarningTimer += deltaTime;

        // 警告播放 1.8 秒后，Boss正式出场
        if (m_bossWarningTimer >= 1.8f) {
            int bossType = 0;
            if (currentLevel == 2) bossType = 0;
            else if (currentLevel == 7) bossType = 1;
            else bossType = 2;

            BossEntity* boss = EntityFactory::instance()->createEntity<BossEntity>();
            boss->initBoss(bossType, getGameData()->getSceneWidth());

            // 【核心修改】：初始位置设置在屏幕正上方悬浮并位于屏幕外，配合 m_vy 下落
            qreal startX = (getGameData()->getSceneWidth() - BossEntity::BOSS_WIDTH) / 2.0;
			boss->setPos(startX, -BossEntity::BOSS_HEIGHT); // 从屏幕上方外部开始，配合其内部逻辑下落进入战场

            getGameData()->setActiveBoss(boss);
            if (getGameView()) {
                getGameView()->addItem(boss);
            }

            connect(boss, &BossEntity::fireBullet, this, &PlaneGameController::onBossFireBullet);
            connect(boss, &BossEntity::fireTargetedBullet, this, &PlaneGameController::onBossFireTargetedBullet);

            m_bossSpawnedThisLevel = true; 
        }
    }

    // 驱动 Boss 物理与轨迹
    BossEntity* boss = getGameData()->getActiveBoss();
    if (boss && boss->isAlive()) {
        boss->updateLogic(deltaTime);
    }
}
