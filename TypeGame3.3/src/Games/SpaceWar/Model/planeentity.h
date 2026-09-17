/* ------------------------------------------------------------------
// 文件名     : planeentity.h
// 功能描述   : 飞机实体类，继承自 GameEntity，支持大图集矩阵切割渲染
------------------------------------------------------------------ */
#pragma once
#include "gameentity.h"
#include "planegameconfig.h"
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QRectF>

class PlaneEntity : public GameEntity
{
    Q_OBJECT
    // 新增：注册受击特效进度属性
    Q_PROPERTY(qreal hitEffectProgress READ hitEffectProgress WRITE setHitEffectProgress NOTIFY hitEffectProgressChanged)

public:
    static constexpr int PLANE_WIDTH = 120;
    static constexpr int PLANE_HEIGHT = 120;

    // 定义运动轨迹枚举
    enum TrajectoryType {
        Bounce,  // 折线反弹
        Sine,    // 正弦波 S 型
        Tracking // 追踪玩家
    };

    PlaneEntity();
    ~PlaneEntity() override = default;

    PlaneEntity(const PlaneEntity&) = delete;
    PlaneEntity& operator=(const PlaneEntity&) = delete;

    // 只需要重写特有的渲染和销毁接口，其余的下落、动画、选中判定全交给基类！
    void paint(QPainter* painter) override;
    void destroy() override;

    float getHp() const { return m_hp; }
    float getMaxHp() const { return m_maxHp; }
    void setMaxHp(float maxHp) { m_maxHp = maxHp; m_hp = maxHp; }
    // 【修改】受到伤害，传入来源玩家指针用于加分
    bool takeDamage(float dmg, class PlayerEntity* player = nullptr);

    // 播放受击动画
    void startSelectedAnimation() override;// 重写专属于太空大战的导弹击中受击特效
	void startHitAnimation(); // 机炮击中动画，区别于打字命中动画
    // 新增：特效进度的 Getter 和 Setter
    qreal hitEffectProgress() const { return m_hitEffectProgress; }
	void setHitEffectProgress(qreal progress); // 0.0 到 1.0，控制受击特效的生命周期

    // 核心：基于时间增量更新物理位置与轨迹计算
    void updateTrajectory(qreal deltaTime, qreal screenWidth, const QPointF& playerPos);

    // 初始化参数设置
    void setTrajectoryType(TrajectoryType type);
	void setVelocity(qreal vx, qreal vy); // 设置初始速度，特别是 vx 用于 Bounce 和 Sine 轨迹
    void setStartPosition(const QPointF& pos);

    // 【新增】设置敌机的阶段(0-2)与类型(0-3)，并在设置后切割加载对应的贴图
	void setStageAndType(int stage, int type); // 接收阶段和类型，并触发切割
    int getStage() const { return m_stage; }

signals:
    // 新增：属性变化信号
    void hitEffectProgressChanged();

private:
    void loadSpriteFrames(); // 重写加载逻辑

private:
    float m_maxHp = 3.0f; // 敌机默认3点血量
    float m_hp = 3.0f;
    int m_scoreValue = 100; // 【新增】敌机的分数值

	TrajectoryType m_trajectoryType = Bounce; // 默认折线反弹轨迹

    // 新增：轻量级粒子结构体
    struct SparkParticle {
        QPointF direction; // 飞溅方向
        QColor color;      // 粒子颜色
        float speed;       // 飞溅速度
        float size;        // 粒子大小
        bool isDebris;     // true: 翻滚的装甲碎片, false: 拉长的火花射线
    };
    bool m_isFatalExploding = false; // 新增：区分是普通机炮刮痧，还是导弹击中的致命过载
    qreal m_hitEffectProgress = 0.0; // 特效生命周期 (0.0 到 1.0)
    QVector<SparkParticle> m_hitSparks; // 存储单次受击产生的粒子

    // 物理与轨迹属性
    qreal m_vx = 0.0;
    qreal m_vy = 0.0;
    qreal m_startX = 0.0;
    qreal m_timeAlive = 0.0; // 存活时间（用于计算正弦波公式中的 t）

    // 【重构】使用单张贴图替换原有的 m_frames 序列
    QPixmap m_texture;
    QPixmap m_flashTexture; // 用于受击闪白的高亮剪影

    // 【新增】阶段与类型标识
    int m_stage = 0;     // 0:外星母舰, 1:虫巢, 2:敌占区
    int m_enemyType = 0; // 0, 1, 2, 3
};