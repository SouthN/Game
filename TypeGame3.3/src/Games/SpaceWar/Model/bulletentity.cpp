#include "bulletentity.h"
#include "resourcemanager.h"
#include "entityfactory.h"
#include <QtMath> // 【修复】：替换 <cmath>，使用 Qt跨平台数学库
#include <QRandomGenerator> // 【新增】用于生成烟雾随机偏移量

BulletEntity::BulletEntity()
{
    m_rect = QRectF(0, 0, BULLET_WIDTH, BULLET_HEIGHT);
	setBoundingRect(m_rect); // 明确设置包围盒，避免默认过小导致的裁剪问题
    setTransformOriginPoint(BULLET_WIDTH / 2.0, BULLET_HEIGHT / 2.0); // 必须设置中心为旋转原点

    // 加载你给出的子弹素材
    m_texture = ResourceManager::instance()->loadPixmap(":/SpaceWar/assets/images/SpaceWar/Bullet.png");
}

void BulletEntity::setTarget(PlaneEntity* target)
{
    m_target = target;
    m_state = 0;
    m_stateTimer = 0.0f;
    m_speed = 0.0f;
    setRotation(0); // 初始朝上

    // 【新增】：重置烟雾粒子系统，防止从对象池复用时出现上一发子弹的残留烟雾
    for (int i = 0; i < MAX_SMOKE_PARTICLES; ++i) {
        m_smokeParticles[i].life = 0.0f;
    }
    m_smokeIndex = 0;
    m_smokeTimer = 0.0f;

}

// 1. 新增设定 Boss 目标的方法
void BulletEntity::setBossTarget(BossEntity* boss, BossPart* part)
{
    m_bossTarget = boss;
    m_bossPartTarget = part;
    m_target = nullptr; // 清空普通敌机目标
    m_state = 0;
    m_stateTimer = 0.0f;
    m_speed = 0.0f;
    setRotation(0);

    // 重置烟雾粒子
    for (int i = 0; i < MAX_SMOKE_PARTICLES; ++i) {
        m_smokeParticles[i].life = 0.0f;
    }
    m_smokeIndex = 0;
    m_smokeTimer = 0.0f;
}

void BulletEntity::setSpawnPosition(const QPointF& playerCenter, bool isLeftWing)
{
    // 从左翼或右翼弹出
    float offsetX = isLeftWing ? -40.0f : 40.0f;
    float startX = playerCenter.x() + offsetX - BULLET_WIDTH / 2.0;
	float startY = playerCenter.y() - 50.0f; // 从飞机中心稍微向上偏移
    setPos(startX, startY);

    // 初始弹出速度：向外、向下
    float vx = isLeftWing ? -200.0f : 200.0f;
    m_velocity = QPointF(vx, 150.0f);

}

void BulletEntity::updateLogic(float deltaTime)
{
    // 【核心修复】：只要普通目标为空，且 Boss 目标也为空，才 return 退出。
    // 换句话说：只要这俩目标有任何一个存在，物理引擎就继续运转！
    if (!m_target && (!m_bossTarget || !m_bossPartTarget)) return;

    m_stateTimer += deltaTime;

    // 通知 QGraphicsScene 即将改变实体局部包围盒大小
    prepareGeometryChange();

    if (m_state == 0) {
        // 阶段 1：向侧下方弹出
        setPos(x() + m_velocity.x() * deltaTime, y() + m_velocity.y() * deltaTime);
        m_velocity.setX(m_velocity.x() * 0.9f); // 水平阻尼减速
        m_velocity.setY(m_velocity.y() - 600.0f * deltaTime); // 垂直减速至0

        if (m_stateTimer > 0.15f) {
            m_state = 1; // 进入悬停
            m_stateTimer = 0.0f;
        }
    }
    else if (m_state == 1) {
        // 阶段 2：缓慢向下飘落，替代原来的完全悬停定格

        // 1. 水平方向继续施加微弱的空气阻力，让抛物线更柔和
        m_velocity.setX(m_velocity.x() * 0.95f);

        // 2. 垂直方向赋予一个缓慢向下的失速（Qt 坐标系 Y 轴正方向为向下）
        // 这里设为 60.0f，你可以根据想要的“掉落感”轻重来微调这个数值
        m_velocity.setY(120.0f);

        // 3. 应用位移（原本这里是空的，导致了绝对悬停）
        setPos(x() + m_velocity.x() * deltaTime, y() + m_velocity.y() * deltaTime);

        // 建议：原本这里的悬停时间是 0.1f (100毫秒)，非常短。
        // 如果你改了缓慢向下后觉得还是看不清这个状态，可以把 0.1f 稍微加大到 0.2f 或 0.25f
        if (m_stateTimer > 0.25f) {
            m_state = 2; // 引擎点火，进入追踪
            m_speed = 200.0f; // 初始爆发速度
        }
    }
    else if (m_state == 2) {
        // 阶段 3：极致加速度追踪目标
        // ====== 修改：支持两种目标的坐标获取 ======
        QPointF targetPos;
        if (m_target) {
            targetPos = m_target->scenePos() + QPointF(PlaneEntity::PLANE_WIDTH / 2.0, PlaneEntity::PLANE_HEIGHT / 2.0);
        }
        else if (m_bossTarget && m_bossPartTarget) {
            // 追踪 Boss 的局部部位：Boss中心点坐标 + 部位局部偏移坐标
            targetPos = m_bossTarget->scenePos() + QPointF(BossEntity::BOSS_WIDTH / 2.0, BossEntity::BOSS_HEIGHT / 2.0) + m_bossPartTarget->localRect.center();
        }
        else {
            return; // 丢失目标则停止更新
        }

        QPointF myPos = scenePos() + QPointF(BULLET_WIDTH / 2.0, BULLET_HEIGHT / 2.0);
        QPointF dir = targetPos - myPos;

        float dist = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (dist > 0) {
            dir /= dist;
        }

        // 超大加速度
        m_speed += 3000.0f * deltaTime;
        m_velocity = dir * m_speed;

		setPos(x() + m_velocity.x() * deltaTime, y() + m_velocity.y() * deltaTime); // 应用位移

        // 计算旋转角度 (atan2返回弧度，需转为角度)
        // Y轴负方向向上，因此需要+90度校准子弹尖端
        float angle = std::atan2(dir.y(), dir.x()) * 180.0 / M_PI;
        setRotation(angle + 90.0);
    }

    // ================= 【新增：烟雾粒子系统更新计算】 =================
    // 1. 更新存活粒子的生命周期
    for (int i = 0; i < MAX_SMOKE_PARTICLES; ++i) {
        if (m_smokeParticles[i].life > 0) {
            m_smokeParticles[i].life -= deltaTime;
        }
    }
    // 2. 尾随生成新粒子 (控制频率约为每 16~20ms 一颗)
    m_smokeTimer += deltaTime;
    if (m_smokeTimer >= 0.016f) {
        m_smokeTimer = 0.0f;

        // 赋予细微的横向抖动偏移，让尾迹看起来更像无序膨胀的浓烟
        float offsetX = (QRandomGenerator::global()->generateDouble() - 0.5) * 8.0;

        // 将子弹的局部尾端坐标，投射映射出此时它处在全局场景的什么位置
        QPointF tailScenePos = mapToScene(QPointF(BULLET_WIDTH / 2.0 + offsetX, BULLET_HEIGHT));

        // 装填入环形数组（循环覆盖机制）
        m_smokeParticles[m_smokeIndex].scenePos = tailScenePos;
		m_smokeParticles[m_smokeIndex].life = 0.2f; // 0.2秒快速消散，防止拖尾过长吃性能
		m_smokeParticles[m_smokeIndex].maxLife = 0.2f; // 最大存活时间 (0.2秒快速消散，防止拖尾过长吃性能)
        m_smokeIndex = (m_smokeIndex + 1) % MAX_SMOKE_PARTICLES;
    }
    // 3. 动态扩张局部的渲染包围盒，确保图形框架不裁剪掉在拖尾远处的烟雾
    m_rect = QRectF(0, 0, BULLET_WIDTH, BULLET_HEIGHT);
    for (int i = 0; i < MAX_SMOKE_PARTICLES; ++i) {
        if (m_smokeParticles[i].life > 0) {
            QPointF localPos = mapFromScene(m_smokeParticles[i].scenePos);
            float radius = 9.0f; // 考虑到烟雾最大膨胀半径
            m_rect = m_rect.united(QRectF(localPos.x() - radius, localPos.y() - radius, radius * 2.0, radius * 2.0));
        }
    }
    // =================================================================
}

void BulletEntity::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // ================= 【新增：绘制烟雾拖尾特效】 =================
    painter->save();
    painter->setPen(Qt::NoPen);
    for (int i = 0; i < MAX_SMOKE_PARTICLES; ++i) {
        if (m_smokeParticles[i].life > 0) {
            float lifeRatio = m_smokeParticles[i].life / m_smokeParticles[i].maxLife;

            // 随着时间流逝：不透明度逐渐降低、半径不断膨胀扩大
            int alpha = static_cast<int>(255 * lifeRatio * 0.7f); // 最高 70% 不透明度
            float radius = 3.0f + (1.0f - lifeRatio) * 6.0f;      // 半径从 3.0 扩散到 9.0

            painter->setBrush(QColor(255, 255, 255, alpha)); // 纯白烟雾

            // 将历史保留在全局的散落点映射回此时此刻因为旋转移动而产生的局部偏移坐标系
            QPointF localPos = mapFromScene(m_smokeParticles[i].scenePos);
            painter->drawEllipse(localPos, radius, radius);
        }
    }
    painter->restore();
    // ============================================================

    // 绘制子弹本体（覆盖在所有特效之上）
    if (!m_texture.isNull()) {
        painter->drawPixmap(0, 0, BULLET_WIDTH, BULLET_HEIGHT, m_texture);
    }
    else {
        // 兜底色块
        painter->setBrush(Qt::cyan);
        painter->drawRect(0, 0, BULLET_WIDTH, BULLET_HEIGHT);
    }
}

void BulletEntity::destroy()
{
	EntityFactory::instance()->destroyEntity<BulletEntity>(this); // 通过工厂回收对象，进入对象池待复用
}