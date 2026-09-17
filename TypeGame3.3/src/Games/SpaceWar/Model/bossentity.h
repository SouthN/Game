/* ------------------------------------------------------------------
// 文件名     : bossentity.h
// 功能描述   : 关卡首领实体，包含多部位破坏机制与渲染
------------------------------------------------------------------ */
#pragma once
#include "gameentity.h"
#include "spriteanimation.h" // 新增包含序列帧动画类
#include <QPixmap>
#include <QVector>
#include <QRectF>
#include <QDateTime>

// 新增：Boss 可破坏部位结构体
struct BossPart {
    int id;               // 部位唯一标识
    QString name;         // 部位名称（如"Left Wing", "Core"）
    QRectF localRect;     // 局部碰撞区域（相对于 Boss 中心点，0,0 为中心）
    float maxHp;            // 该部位最大血量
    float currentHp;        // 该部位当前血量
    bool isExposed;       // 是否被机枪打破装甲，露出字母
    bool isDestroyed;     // 是否已被飞弹彻底摧毁
    QChar letter;         // 破甲后分配的击杀字母

    // ======== 新增以下两行 ========
    bool isLocked;        // 是否被玩家敲击锁定（飞弹正在飞去）
    qint64 lockTime;      // 锁定的时间戳，用于播放极速收缩动画
    // ======== 新增属性 ========
    qreal flashOpacity;   // 当前部位的独立受击闪白透明度
};

class BossEntity : public GameEntity
{
    Q_OBJECT
        Q_PROPERTY(qreal hitEffectProgress READ hitEffectProgress WRITE setHitEffectProgress NOTIFY hitEffectProgressChanged)

public:
    BossEntity();
    ~BossEntity() override = default;

    static constexpr int BOSS_WIDTH = 450;
    static constexpr int BOSS_HEIGHT = 400;

    void paint(QPainter* painter) override;
    void destroy() override;
    void updateLogic(float deltaTime = 0.0f) override;

    void initBoss(int type, qreal sceneWidth);

    // 废弃原本的整体 takeDamage，改为基于局部坐标的部位伤害判定
    // 返回被击中的部位指针，如果没有击中有效部位则返回 nullptr
    BossPart* checkAndDamagePart(const QPointF& localPos, float dmg);

    int getBossType() const { return m_bossType; }

    // 获取所有部位信息（供控制器分配字母和检测摧毁进度）
    QList<BossPart>& getParts() { return m_parts; }
    bool areAllPartsDestroyed() const;

    // 2. 修改 startHitAnimation 的声明，增加 hitPart 指针参数（默认值为 nullptr）
    void startHitAnimation(const QPointF& localHitPos, BossPart* hitPart = nullptr);
    void startSelectedAnimation() override; 

    // 新增：触发局部爆炸动画的接口
    void playPartExplosion(BossPart* part);

    // ====== 新增：濒死连环爆炸状态 ======
    void startDefeatedSequence();
    bool isDying() const { return m_isDying; }

    qreal hitEffectProgress() const { return m_hitEffectProgress; }
    void setHitEffectProgress(qreal progress);

signals:
    // ====== Boss 攻击信号 ======
    // 普通定向弹幕（军方、泽塔星系）
    void fireBullet(int bulletType, const QPointF& startPos, const QPointF& velocity, float damage);
    // 追踪弹幕（异虫，不需要提供速度方向，只需告知初始位置和速率，由 Controller 计算朝向）
    void fireTargetedBullet(int bulletType, const QPointF& startPos, float speed, float damage);

    // ====== 新增：连环爆炸音效与死亡结算信号 ======
    void chainExplosionTriggered();
    void deathSequenceFinished();
	void hitEffectProgressChanged(); // 打击动画进度变化，供界面更新

private:
    int m_bossType = 0;

    // 新增：部位列表
    QList<BossPart> m_parts;

    qreal m_vx = 150.0;
    qreal m_vy = 160.0;
    qreal m_sceneWidth = 1000.0;

    // ====== 攻击系统变量 ======
    float m_attackTimer = 0.0f;       // 攻击冷却计时器
    float m_attackInterval = 2.0f;    // 攻击间隔（可动态改变）
    int m_attackPhase = 0;            // 攻击阶段/轮次（用于切换弹幕形态）

    // 执行攻击弹幕的内部函数
    void processAttack(float deltaTime);

	// 受击动画状态
    QPixmap m_texture;
    QPixmap m_flashTexture;
    QPixmap m_damagedTexture; // 存放完全损坏版本的 Boss 贴图

    struct SparkParticle {
        QPointF direction;
        QColor color;
        float speed;
        float size;
        bool isDebris;
        QPointF startPos;
    };
    bool m_isFatalExploding = false;
    qreal m_hitEffectProgress = 0.0;
    QVector<SparkParticle> m_hitSparks;

	// 部位破坏动画状态
    // ======== 新增：序列帧爆炸相关 ========
    struct ActiveExplosion {
        QPointF localPos;       // 爆炸相对于 Boss 中心的坐标
        SpriteAnimation* anim;  // 绑定的动画实例
    };
    QList<ActiveExplosion> m_activeExplosions; // 当前正在播放的爆炸动画列表
    QVector<QPixmap> m_explosionFrames;        // 当前 Boss 阵营对应的爆炸序列帧缓存

    // 完全摧毁动画状态
    // ====== 新增：濒死计时器变量 ======
    bool m_isDying = false;
    float m_deathTimer = 0.0f;
    float m_explosionEffectTimer = 0.0f;
};