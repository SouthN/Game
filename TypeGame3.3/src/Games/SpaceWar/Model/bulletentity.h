/* ------------------------------------------------------------------
// 文件名     : bulletentity.h
// 功能描述   : 追踪子弹实体，负责阶段性物理运动（弹出->悬停->追踪）及姿态旋转
------------------------------------------------------------------ */
#pragma once
#include "gameentity.h"
#include "planeentity.h"
// 在头部增加包含
#include "bossentity.h"

class BulletEntity : public GameEntity
{
    Q_OBJECT
public:
    static constexpr int BULLET_WIDTH = 25;
    static constexpr int BULLET_HEIGHT = 67;

    BulletEntity();
    ~BulletEntity() override = default;

    void paint(QPainter* painter) override;
    void destroy() override;
    void updateLogic(float deltaTime = 0.0f) override;

    // 核心接口
    void setTarget(PlaneEntity* target);
    PlaneEntity* getTarget() const { return m_target; }

    // ====== 新增：支持锁定 Boss 的局部部位 ======
    void setBossTarget(BossEntity* boss, BossPart* part);
    BossEntity* getBossTarget() const { return m_bossTarget; }
    BossPart* getBossPartTarget() const { return m_bossPartTarget; }

    void setSpawnPosition(const QPointF& playerCenter, bool isLeftWing);

private:
    PlaneEntity* m_target = nullptr;

    // ====== 新增：Boss 追踪目标 ======
    BossEntity* m_bossTarget = nullptr;
    BossPart* m_bossPartTarget = nullptr;

    QPixmap m_texture; // 子弹纹理

    // 状态机
    int m_state = 0; // 0: 侧边弹出, 1: 悬停, 2: 冲刺追踪
	float m_stateTimer = 0.0f; // 当前状态持续时间计时器

    // 物理属性
	QPointF m_velocity; // 当前速度向量
	float m_speed = 0.0f; // 当前速度大小（根据状态调整）

    // ================= 【新增：极轻量烟雾粒子系统】 =================
    struct SmokeParticle {
        QPointF scenePos;  // 记录产生时所在的世界全局坐标
        float life = 0.0f; // 当前生命周期
        float maxLife = 0.2f; // 最大存活时间 (0.2秒快速消散，防止拖尾过长吃性能)
    };
    static constexpr int MAX_SMOKE_PARTICLES = 15; // 环形队列最大容量
	SmokeParticle m_smokeParticles[MAX_SMOKE_PARTICLES]; // 固定大小数组，避免动态内存分配
    int m_smokeIndex = 0;
    float m_smokeTimer = 0.0f;
    // ================================================================
};