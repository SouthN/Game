/* ------------------------------------------------------------------
// 文件名     : playerentity.cpp
// ------------------------------------------------------------------ */
#include "playerentity.h"
#include "planegameconfig.h" // 确保引入了配置单例
#include <QPainter>
#include <QPainterPath>         // 新增：用于绘制不规则火焰多边形
#include <QRandomGenerator>     // 新增：用于生成随机数实现闪烁
#include <QDebug>
#include <QtMath>
#include <QPropertyAnimation>



PlayerEntity::PlayerEntity() : GameEntity()
{
    // 初始化物理包围盒大小
    m_rect = QRectF(0, 0, PLAYER_WIDTH, PLAYER_HEIGHT); 
	
    // 【新增】：初始化挂载属性组件
    m_attributes = new PlayerAttributeComponent(this);
}

PlayerEntity::~PlayerEntity()
{
    // 无需手动停止动画，QVector 会自动释放其内容
    // m_attributes 是本对象的子对象(QObject树)，会自动释放
}

void PlayerEntity::loadTexture(const QString& spriteSheetPath)
{
    QPixmap spriteSheet(spriteSheetPath);
    if (spriteSheet.isNull()) {
        qWarning() << "[PlayerEntity] Player Entity Load Fail，Path:" << spriteSheetPath;
        return;
    }

    // 素材由 5 张动作序列构成，在此进行横向等分切割
    const int frameCount = 5;
    int frameWidth = spriteSheet.width() / frameCount;
    int frameHeight = spriteSheet.height();

    m_frames.clear();
    for (int i = 0; i < frameCount; ++i) {
        // copy(x, y, width, height) 提取单帧贴图并存入数组
        QPixmap singleFrame = spriteSheet.copy(i * frameWidth, 0, frameWidth, frameHeight);
        m_frames.append(singleFrame);
    }
}

void PlayerEntity::setMovingDirection(int dir)
{
    // 只更新“目标状态”，不直接修改当前帧
    // 素材参考：0:全速右倾, 1:轻微右倾, 2:居中, 3:轻微左倾, 4:全速左倾
    if (dir < 0) {
        m_targetFrameIndex = 4; // 意图向左
    }
    else if (dir > 0) {
        m_targetFrameIndex = 0; // 意图向右
    }
    else {
        m_targetFrameIndex = 2; // 无按键，保持居中平飞
    }

}

void PlayerEntity::updateLogic(float deltaTime)
{
    // 不调用基类的 GameEntity::updateLogic(deltaTime)，因为我们不需要战机自动下坠

    // 如果当前姿态还没有到达目标姿态，则进行过渡计算
    if (m_currentFrameIndex != m_targetFrameIndex) {
        // 累加经过的时间
        m_frameTimer += deltaTime;

        // 达到设定的阈值时间（例如 50ms）
        if (m_frameTimer >= FRAME_TRANSITION_DELAY) {
            m_frameTimer = 0.0f; // 重置计时器

            // 向目标帧逼近一步（左倾或右倾）
            if (m_currentFrameIndex < m_targetFrameIndex) {
                m_currentFrameIndex++;
            }
            else {
                m_currentFrameIndex--;
            }
            // 通知 QGraphicsScene 针对当前包围盒区域进行重绘
            update();
        }
    }
    else {
        // 已经到达目标姿态，计时器安静清零
        m_frameTimer = 0.0f;
    }

}

void PlayerEntity::startShieldAnimation()
{
    // 强制终止之前的护盾状态，防止连续受击动画错乱
    setShieldProgress(0.0);

    // 【核心新增】：开启无敌状态
    setInvincible(true);

    QPropertyAnimation* shieldAnim = new QPropertyAnimation(this, "shieldProgress", this);
    // 【核心修改】：通过属性组件获取无敌持续时间，替代原有的常量宏
    shieldAnim->setDuration(m_attributes->invincibleDurationMs());
    // 动画曲线：瞬间充能达到最亮，然后缓慢消散
    shieldAnim->setKeyValueAt(0.0, 0.0);
    shieldAnim->setKeyValueAt(0.05, 1.0); // 瞬间达到最大亮度
    shieldAnim->setKeyValueAt(1.0, 0.0);   // 1000ms: 最后 150ms 瞬间断电、迅速崩塌消失
	shieldAnim->setEasingCurve(QEasingCurve::OutQuad); // 以平滑的二次衰减曲线模拟能量耗尽的自然感觉

    // 【核心新增】：动画完全结束时，自动解除无敌状态
    connect(shieldAnim, &QPropertyAnimation::finished, this, [this]() {
        setInvincible(false);
        });

    shieldAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void PlayerEntity::setShieldProgress(qreal progress)
{
    if (qFuzzyCompare(m_shieldProgress, progress)) return;
    m_shieldProgress = qBound(0.0, progress, 1.0);
    update(); // 进度变化时触发重绘
    emit shieldProgressChanged();
}

void PlayerEntity::paint(QPainter* painter)
{
    if (!m_isAlive) return;

    // ========== 【新增代码】：绘制玩家蓝色尾焰 ==========
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 随机生成一个 0.8 ~ 1.2 的缩放值，模拟尾焰不稳定闪烁(Flicker)
    qreal flicker = 0.8 + (QRandomGenerator::global()->generateDouble() * 0.4);

    // 1. 将画笔原点平移到飞机尾部正中央 (根据 PLAYER_WIDTH=100, PLAYER_HEIGHT=130 微调)
    painter->translate(PLAYER_WIDTH / 2.0, PLAYER_HEIGHT - 35);
    painter->scale(1.0, flicker); // 仅在 Y 轴动态缩放实现喷射感

    // 2. 勾勒火焰形状 (一个倒置的泪滴/尖刺形)
    QPainterPath flamePath;
    flamePath.moveTo(-12, 0); // 喷射口左侧
    flamePath.lineTo(12, 0);  // 喷射口右侧
    flamePath.lineTo(0, 50);  // 火焰尖端 (向下延伸 50 像素)

    // 3. 使用线性渐变模拟真实火焰温度分布 (白心 -> 科幻蓝 -> 透明)
    QLinearGradient gradient(0, 0, 0, 50);
    gradient.setColorAt(0.0, QColor(200, 255, 255, 255)); // 核心高亮白
    gradient.setColorAt(0.4, QColor(50, 150, 255, 200));  // 中段科幻蓝
    gradient.setColorAt(1.0, QColor(0, 0, 255, 0));       // 尾部透明消散

    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawPath(flamePath);
    painter->restore();
    // ====================================================

    // 绘制本体,从数组中取出当前帧绘制，加入越界保护
    if (!m_frames.isEmpty() && m_currentFrameIndex >= 0 && m_currentFrameIndex < m_frames.size()) {
        painter->drawPixmap(m_rect.toRect(), m_frames[m_currentFrameIndex]);
    }

    // 绘制蓝色电磁能量护盾
    if (m_shieldProgress > 0.0) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // 开启加法混合模式，让护盾呈现出高亮的能量发光质感
        painter->setCompositionMode(QPainter::CompositionMode_Plus);

        // 将原点移动到战机中心
        QPointF center(PLAYER_WIDTH / 2.0, PLAYER_HEIGHT / 2.0);
        painter->translate(center);

        // 护盾基础半径，随进度有轻微的张弛呼吸感
        float radius = 65.0f + (1.0f - m_shieldProgress) * 15.0f;
        int alpha = static_cast<int>(255 * m_shieldProgress);

        // --- 1. 外层：深空蓝实线力场 ---
        QPolygonF outerHexagon;
        for (int i = 0; i < 6; ++i) {
            qreal angle = i * M_PI / 3.0;
            outerHexagon << QPointF(radius * std::cos(angle), radius * std::sin(angle));
        }

        // 贴合尾焰的深空蓝色
        QPen outerPen(QColor(50, 150, 255, alpha), 3.0f);
        painter->setPen(outerPen);

        int fillAlpha = static_cast<int>(60 * m_shieldProgress);
        painter->setBrush(QColor(0, 100, 255, fillAlpha)); // 内部电磁蓝微光填充

        painter->save();
        painter->rotate(m_shieldProgress * 90.0); // 外层顺时针旋转
        painter->drawPolygon(outerHexagon);
        painter->restore();

        // --- 2. 内层：亮青色虚线电磁环 ---
        float innerRadius = radius * 0.85f; // 内圈稍微小一点
        QPolygonF innerHexagon;
        for (int i = 0; i < 6; ++i) {
            qreal angle = i * M_PI / 3.0;
            innerHexagon << QPointF(innerRadius * std::cos(angle), innerRadius * std::sin(angle));
        }

        // 亮青色虚线，模拟高频跳动的电流
        QPen innerPen(QColor(0, 255, 255, alpha), 2.0f, Qt::DashLine);
        painter->setPen(innerPen);
        painter->setBrush(Qt::NoBrush);

        painter->save();
        painter->rotate(-m_shieldProgress * 180.0); // 内层逆时针高速旋转，产生磁场交错感
        painter->drawPolygon(innerHexagon);
        painter->restore();

        painter->restore();
    }
    // ====================================================================

}

void PlayerEntity::destroy()
{
    m_isAlive = false;
    // 隐藏图元，脱离渲染管线，等待 Controller 回收或对象池复用
    hide();
}

// Qt 在进行 collidesWithItem 碰撞检测时，会优先使用 shape() 返回的精确路径。
QPainterPath PlayerEntity::shape() const
{
    QPainterPath path;
    // 原始战机大小为 100x130 (PLAYER_WIDTH x PLAYER_HEIGHT)
    // 我们将碰撞判定盒向内缩小：例如左右各缩进 60 像素，上下各缩进 60 像素
    // 这样玩家只有战机的核心机身部分才会被判定为受击，边缘机翼就算重叠也不会扣血
    QRectF hitBox(60, 60, PLAYER_WIDTH - 120, PLAYER_HEIGHT - 120);
    path.addRect(hitBox);

    return path;
}

// ================= 【重构核心：代理转发给属性组件】 =================
void PlayerEntity::initHealth(float maxHp) {
    m_attributes->setMaxHp(maxHp);
    m_attributes->setHp(maxHp);
}

bool PlayerEntity::takeDamage(float dmg) {
    if (!m_isAlive) return false;
    // 调用组件计算并扣除实际伤害（包含了护甲减伤逻辑）
    m_attributes->applyDamage(dmg);
    // 从组件读取最新血量判断是否坠毁
    if (m_attributes->hp() <= 0.0f) {
        return true; // 玩家坠毁
    }
    return false;
}

void PlayerEntity::heal(float amount)
{
    if (!m_isAlive) return;
    // 恢复量由组件安全把控，不超上限
    m_attributes->heal(amount);
}

void PlayerEntity::addScore(int score)
{
    // 将获得的分数上报给全局配置以更新 UI
    PlaneGameConfig::instance()->addScore(score);
}
