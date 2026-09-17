/* ------------------------------------------------------------------
// 文件名     : planegamecontroller.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 太空大战逻辑控制器，处理敌机生成、下落、键盘匹配及胜负判定
------------------------------------------------------------------ */
#pragma once
#include "gamecontrollerbase.h"
#include "planegamedata.h"
#include "planegameview.h"
#include "bulletentity.h"
#include "enemybulletentity.h"
#include "machinegunbulletentity.h"
#include "wordgenerator.h"
#include "rewardwordentity.h"
#include <QList>

// 太空大战游戏控制器（MVC-Controller）
class PlaneGameController : public GameControllerBase
{
    Q_OBJECT
public:
    explicit PlaneGameController(PlaneGameData* data, PlaneGameView* view, QObject* parent = nullptr);
    void initConnection(PlaneGameData* data, PlaneGameView* view);
    ~PlaneGameController() override = default;

	void handleKeyPress(QKeyEvent* event) override; // 重写键盘事件处理，接管玩家输入逻辑
	void stopGame() override; // 重写停止游戏，清理敌机和子弹实体，重置状态
	void startGame() override; // 重写开始游戏，重置状态并调用基类逻辑

public slots:
    // ========= 新增：重写下一关逻辑，用于清理子弹 =========
    void onNextLevelClicked() override;

protected:
    // =========================================================
    // 实现基类的模板方法钩子
    // =========================================================
	GameEntity* createNewEntity() override;  // 生成新的敌机实体
	void updateViewEntities() override; // 更新视图中的敌机位置和状态
    void removeEntityFromView(GameEntity* entity) override;
    void clearSuccessIconsFromView() override;
    void playHitSound() override;
    void playMissSound() override;
    int getEntityWidth() const override;
    int getEntityHeight() const override;
    int getSpawnEntityCount() const override;

	// 【新增】重写自定义物理逻辑，接管玩家战机的输入与移动，敌人战机的复杂轨迹计算
    void updateCustomPhysics(float deltaTime) override; 
    // 【新增】重写基类的过关判定，拦截弹窗并插入动画时间
    void checkPassCondition() override;

private:
    PlaneGameData* getGameData() { return static_cast<PlaneGameData*>(m_gameData); }
    PlaneGameView* getGameView() { return static_cast<PlaneGameView*>(m_gameView); }

	// ====== 追踪子弹管理列表 ======
	QList<BulletEntity*> m_activeBullets; // 当前场景中所有活跃的追踪子弹实例，Controller 负责管理它们的生命周期和逻辑更新
	bool m_spawnLeftWing = true; // 用于交替左右翼发射追踪子弹的标志

    // ====== 机炮子弹管理列表 ======
    QList<MachineGunBulletEntity*> m_activeMachineGunBullets; // 机炮子弹活跃列表
    // ======== 连发冷却控制 ========
    float m_fireCooldown = 0.0f;
    static constexpr float FIRE_RATE = 0.15f; // 射击间隔：0.15秒一发 (大约一秒7发)


    // ====== 新增：Boss子弹管理列表 ======
    QList<EnemyBulletEntity*> m_enemyBullets;
    // ====== 逻辑子模块 ======
    void updateEnemyBulletPhysics(float deltaTime); // 处理敌方子弹移动与碰撞
    // ====== 响应 Boss 开火的内部槽函数 ======
    void onBossFireBullet(int type, const QPointF& pos, const QPointF& vel, float dmg);
    void onBossFireTargetedBullet(int type, const QPointF& pos, float speed, float dmg);

	// ======== 奖励单词生成与管理 ========
    RewardWordEntity* m_rewardEntity = nullptr; 
	float m_rewardTimer = 0.0f; // 奖励单词生成计时器，控制生成频率
	bool m_isGeneratingWord = false; // 是否正在生成奖励单词，控制生成频率和避免重复生成
    
    // =========================================================
    // 物理与逻辑更新的子模块封装，提升可读性并遵循单一职责原则
    // =========================================================
    void updatePlayerPhysicsAndShooting(float deltaTime);
    void updateEnemiesAndPlayerCollision(float deltaTime);
    void updateGuidedBullets(float deltaTime);
    void updateMachineGunBullets(float deltaTime);
    void updateRewardWords(float deltaTime);
	void updateBossPhysics(float deltaTime); // 【新增】Boss战专用物理更新方法，处理Boss的特殊移动模式和攻击逻辑
	void cleanupBoss(); // 【新增】Boss战结束时的清理方法，停止动画、剥离图元、销毁实体等
    void releaseBossLetters(BossEntity* boss); // 释放 Boss 部位占用的全局字母
    
	// Boss战相关状态变量
    bool m_bossSpawnedThisLevel = false;     // 控制每关只生成一次 Boss
    bool m_isBossPhase = false;           // 是否处于Boss降临和战斗阶段
    float m_bossWarningTimer = 0.0f;      // 警告等待倒计时
};
