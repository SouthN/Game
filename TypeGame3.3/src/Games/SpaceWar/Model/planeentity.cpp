#include "planeentity.h"
#include "playerentity.h"
#include "resourcemanager.h"
#include "entityfactory.h" 
#include <QPainterPath>
#include <QRandomGenerator>     // 新增：用于生成随机数
#include <QtMath>

PlaneEntity::PlaneEntity()
{
    m_rect = QRectF(0, 0, PLANE_WIDTH, PLANE_HEIGHT);
	setBoundingRect(m_rect); // 设置边界矩形，供 QGraphicsScene 进行碰撞检测和重绘优化
    
    // 初始默认速度（垂直下落基准速度由 Config 提供）
    m_vy = PlaneGameConfig::instance()->getFallSpeed();
    m_vx = 0.0;

    // 注意：这里的构造函数不再直接调用 loadSpriteFrames()
    // 而是等待 Controller 分配阶段和类型后调用 setStageAndType 触发
}

void PlaneEntity::destroy()
{
    // 将对象交还给内存池回收
    EntityFactory::instance()->destroyEntity<PlaneEntity>(this);
}

void PlaneEntity::updateTrajectory(qreal deltaTime, qreal screenWidth, const QPointF& playerPos)
{
    // 【修改点】：如果飞机已死亡，或者被击中正处于“冻结”动画状态，则停止位置更新
    if (!isAlive() || property("hitFrozen").toBool()) return;

    m_timeAlive += deltaTime;
    qreal prevX = x();
    qreal nextX = x();
    qreal nextY = y() + m_vy * deltaTime;

    // 根据枚举执行对应的轨迹方程
    switch (m_trajectoryType) {
    case Bounce: {
        nextX += m_vx * deltaTime;
        // 修复边界反弹逻辑（以左上角 x 为基准）
        if (nextX < 0) { // 左边界
            nextX = 0;
            m_vx = std::abs(m_vx); // 触碰左边界，强制向右
        }
        else if (nextX + PLANE_WIDTH > screenWidth) { // 右边界
            nextX = screenWidth - PLANE_WIDTH;
            m_vx = -std::abs(m_vx); // 触碰右边界，强制向左
        }
        break;
    }
    case Sine: {
        // 【关键修改】：动态计算振幅，确保 S 弯能覆盖整个屏幕宽度
        // 振幅 = (屏幕总宽 - 飞机宽度) / 2
        qreal maxAmplitude = (screenWidth - PLANE_WIDTH) * 0.5;
        qreal amplitude = maxAmplitude * 0.9; // 取 90% 的可用空间，留一点容错
        qreal omega = 1.2;

        // 以屏幕中心为基准进行摆动，而不是以出生点 m_startX
        qreal centerX = (screenWidth - PLANE_WIDTH) * 0.5;
        nextX = centerX + amplitude * std::sin(omega * m_timeAlive);

        if (deltaTime > 0) m_vx = (nextX - prevX) / deltaTime;
        break;
    }
    case Tracking: {
        // 1. 获取准确的中心点坐标进行比较，使得追踪视觉上更精准
        qreal planeCenterX = x() + PLANE_WIDTH / 2.0;
        // 假设玩家宽度大概也是 100 左右，加上 50 粗略对齐中心点
        qreal playerCenterX = playerPos.x() + 50.0;
        qreal diffX = playerCenterX - planeCenterX;

        // 2. 设定合理的极限追踪速度
        // 引入保底速度 150.0，防止由于配置中 m_vy 过小导致根本没有横向动力
        qreal maxTrackSpeed = std::abs(m_vy) * 1.5;
        if (maxTrackSpeed < 150.0) {
            maxTrackSpeed = 150.0;
        }

        qreal targetVx = 0.0;

        // 3. 增加死区判定 (>10.0)，防止敌机在玩家正上方时疯狂左右抽搐
        if (std::abs(diffX) > 10.0) {
            targetVx = (diffX > 0) ? maxTrackSpeed : -maxTrackSpeed;
        }

        // 4. 【核心修复】：使用绝对安全的缓动插值算法
        // 每次更新向目标速度靠拢 5%，不仅丝滑，而且永远不会引发浮点数震荡和溢出
        m_vx = m_vx * 0.95 + targetVx * 0.05;

        // 真实的物理位移依然带上 deltaTime，保证不同帧率下移动距离一致
        nextX += m_vx * deltaTime;
        break;
    }
    }
    setPos(nextX, nextY);
}

void PlaneEntity::setTrajectoryType(TrajectoryType type)
{
    m_trajectoryType = type;
}

void PlaneEntity::setVelocity(qreal vx, qreal vy)
{
    m_vx = vx;
    m_vy = vy;
}

void PlaneEntity::setStartPosition(const QPointF& pos)
{
    setPos(pos);
    m_startX = pos.x(); // 记录初始 X 坐标，供正弦波使用
}

void PlaneEntity::setStageAndType(int stage, int type)
{
    m_stage = stage;
    m_enemyType = type;
    loadSpriteFrames();
}

void PlaneEntity::loadSpriteFrames()
{
    // 注意：请将包含12等分敌人的大图放入对应目录
    QString imgPath = ":/SpaceWar/assets/images/SpaceWar/EnemyPlane3.png";
    QPixmap spriteSheet = ResourceManager::instance()->loadPixmap(imgPath);

    if (!spriteSheet.isNull()) {
        // 3行4列
        int frameWidth = spriteSheet.width() / 4;
        int frameHeight = spriteSheet.height() / 3;

        // 根据 stage (行) 和 type (列) 提取贴图
        QPixmap frame = spriteSheet.copy(m_enemyType * frameWidth, m_stage * frameHeight, frameWidth, frameHeight);

        // 缩放到物理碰撞盒大小
        m_texture = frame.scaled(PLANE_WIDTH, PLANE_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // 生成白底闪光剪影（SourceIn 混合模式保留 Alpha 通道）
        QPixmap flashFrame(m_texture.size());
        flashFrame.fill(Qt::transparent);
        QPainter p(&flashFrame);
		p.drawPixmap(0, 0, m_texture); // 先绘制原图到闪光帧
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(flashFrame.rect(), Qt::white);
        p.end();

        m_flashTexture = flashFrame;
    }
}


void PlaneEntity::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(m_opacity);
    painter->save(); 

    QRectF bounds = boundingRect();
    QPointF center = bounds.center();
    painter->translate(center);
    painter->scale(m_scaleX, m_scaleY);
    painter->translate(-center);


    // 【修改】直接绘制静态贴图
    if (!m_texture.isNull()) {
        QPointF topLeft = bounds.center() - QPointF(m_texture.width() / 2.0, m_texture.height() / 2.0);
        painter->drawPixmap(topLeft, m_texture);
    }
    else {
        painter->setBrush(!isAlive() ? Qt::red : Qt::blue);
        painter->drawRoundedRect(bounds, 10, 10);
    }

    // 闪白效果
    if (m_flash > 0.0 && !m_flashTexture.isNull()) {
        painter->save();
        painter->setOpacity(m_flash * 0.9);
        QPointF topLeft = bounds.center() - QPointF(m_flashTexture.width() / 2.0, m_flashTexture.height() / 2.0);
        painter->drawPixmap(topLeft, m_flashTexture);
        painter->restore();
    }

    // 绘制打字通的核心字母机制
    // 绘制全息 HUD 锁定与数据面板 (Holo-Hex 矩阵风格)
    if (isAlive()) {
        painter->save();

        qreal t = m_timeAlive;

        // 锁定动画进度 (在 0.4 秒内快速完成锁定)
        qreal lockProgress = qMin(t * 2.5, 1.0);
        // 缩放曲线：从 2.5 倍收缩到 1.0 倍，带有极速压迫感
        qreal lockScale = 1.0 + (1.0 - lockProgress) * 1.5;

        // 锁定完成后的呼吸灯脉冲效果
        qreal pulse = (std::sin(t * 5.0) + 1.0) * 0.5;

        // 颜色定义 (科技冰蓝 + 能量亮青)
        QColor hexColor(0, 191, 255); // 深天蓝 (Deep Sky Blue)
        hexColor.setAlpha(150 + static_cast<int>(80 * pulse));
        QColor coreColor(0, 255, 204); // 亮青色 (Bright Cyan)
        coreColor.setAlpha(static_cast<int>(255 * lockProgress)); // 锁定完成后内环才完全显现

        QPointF center = bounds.center();

        // --- 1. 动态六边形锁定阵列 ---
        if (isSelected()) { // <--- 核心修改：增加状态拦截，只有被玩家打字锁定后才显示
            painter->save();
            painter->translate(center);
            painter->scale(lockScale, lockScale);

            // 外层六边形持续顺时针缓慢旋转
            painter->rotate(t * 60.0);

            // 绘制虚线六边形外环
            QPen hexPen(hexColor, 2, Qt::DashLine);
            painter->setPen(hexPen);
            painter->setBrush(Qt::NoBrush);

            int r = 48; // 外环半径
            QPolygonF hexagon;
            for (int i = 0; i < 6; ++i) {
                qreal angle = i * 3.1415926 / 3.0; // 60度间隔
                hexagon << QPointF(r * std::cos(angle), r * std::sin(angle));
            }
            painter->drawPolygon(hexagon);

            // 内侧倒三角瞄准核心 (逆时针反向旋转，速度更快)
            painter->rotate(-t * 150.0);
            QPen corePen(coreColor, 1.5);
            painter->setPen(corePen);

            int tr = 22; // 内环半径
            QPolygonF triangle;
            for (int i = 0; i < 3; ++i) {
                // -90度偏移，确保三角形的一个尖端始终作为视觉引导
                qreal angle = i * 2.0 * 3.1415926 / 3.0 - 3.1415926 / 2.0;
                triangle << QPointF(tr * std::cos(angle), tr * std::sin(angle));
            }
            painter->drawPolygon(triangle);

            painter->restore();
        }

        // --- 2. AR 视觉数据引线 ---
        // 为了绝对不遮挡机头，向右下方大角度引出 (X正向，Y正向)
        QPointF startNode = center + QPointF(35, 35);
        QPointF elbowNode = startNode + QPointF(20, 30);
        QPointF textNode = elbowNode + QPointF(45, 0);

        QPen linePen(hexColor, 2);
        painter->setPen(linePen);

        // 绘制折线
        painter->drawLine(startNode, elbowNode);
        painter->drawLine(elbowNode, textNode);

        // 节点装饰：引线起点的能量球
        painter->setBrush(coreColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(startNode, 3, 3);
        painter->drawEllipse(elbowNode, 2, 2);
        // 终点光标点
        painter->drawEllipse(textNode, 2, 2);

        // --- 3. 悬浮字母投影 ---
        QFont font = painter->font();
        font.setFamily("Consolas"); // 等宽科技字体
        font.setPixelSize(26);      // 放大字号
        font.setBold(true);
        font.setItalic(true);       // 斜体增加动感
        painter->setFont(font);

        // 文本区域刚好贴在最后一段横线的正上方
        QRectF textRect(elbowNode.x(), elbowNode.y() - 32, 45, 30);

        // 绘制一个科幻感十足的渐变底色 (从左到右渐隐消失)
        QLinearGradient bgGradient(textRect.topLeft(), textRect.topRight());
        bgGradient.setColorAt(0.0, QColor(0, 40, 60, 180));
        bgGradient.setColorAt(1.0, QColor(0, 40, 60, 0));
        painter->fillRect(textRect, bgGradient);

        // 利用错位绘制法，先画一层黑色阴影，增加文字在复杂背景下的辨识度
        painter->setPen(QColor(0, 0, 0, 180));
        painter->drawText(textRect.translated(2, 2), Qt::AlignCenter, m_letter);

        // 再绘制纯白色的主体文字
        painter->setPen(QColor(255, 255, 255));
        painter->drawText(textRect, Qt::AlignCenter, m_letter);

        painter->restore();
    }

    painter->restore();

    // ================= 【物理碰撞与毁灭打击双轨渲染】 =================
    if (m_hitEffectProgress > 0.0 && m_hitEffectProgress < 1.0) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QRectF bounds = boundingRect();
        QPointF center = bounds.center();
        painter->translate(center);

        float alphaFade = 1.0f - m_hitEffectProgress;

        // 【第一层：普通混合模式】画焦黑碎片、虫族浓血和碎肉
        for (const SparkParticle& s : m_hitSparks) {
            if (s.isDebris || m_stage == 1) { // 虫族所有体液均不使用发光模式，体现生物实体感
                // 基础位移
                float vx = s.direction.x() * s.speed;
                float vy = s.direction.y() * s.speed;
                QPointF currentPos(vx * m_hitEffectProgress, vy * m_hitEffectProgress);

                float dynamicAngle = 0.0f;

                // 虫族血液和肉块受强烈重力下坠，表现黏稠和质量感
                if (m_stage == 1) {
                    float gravityY = 250.0f * m_hitEffectProgress * m_hitEffectProgress; // 二次抛物线下坠
                    currentPos.setY(currentPos.y() + gravityY);

                    // 动态计算受重力影响后的实际运动方向，用于液体拉伸对齐
                    float currentVy = vy + (500.0f * m_hitEffectProgress); // 当前瞬时垂直速度
                    dynamicAngle = std::atan2(currentVy, vx) * 180.0 / M_PI;
                }

                QColor c = s.color;
                // 血液在后期氧化变暗并逐渐透明
                if (m_stage == 1 && !s.isDebris) {
                    c = c.darker(100 + m_hitEffectProgress * 50);
                }
                c.setAlphaF(alphaFade);

                painter->setBrush(c);
                painter->setPen(Qt::NoPen);

                painter->save();
                painter->translate(currentPos);

                if (m_stage == 1 && !s.isDebris) {
                    // 【流体力学渲染】：将血液拉伸成飞溅的液滴状，速度越快拉得越长
                    painter->rotate(dynamicAngle);
                    float stretch = 1.0f + (s.speed * 0.015f) * (1.0f - m_hitEffectProgress);
                    painter->drawEllipse(QRectF(-s.size * stretch, -s.size, s.size * stretch * 2.5f, s.size * 1.5f));
                }
                else {
                    // 固体肉块和碎片疯狂翻滚
                    painter->rotate(m_hitEffectProgress * (s.size > 8.0f ? 540.0 : 1080.0)); // 大肉块翻滚慢
                    painter->drawRoundedRect(-s.size, -s.size, s.size * 2, s.size * 2, s.size * 0.3, s.size * 0.3); // 肉块边缘圆润一点
                }
                painter->restore();
            }
        }

        // 【第二层：加法混合模式 (光效)】画军方和外星人的高亮火花、等离子泄漏
        painter->setCompositionMode(QPainter::CompositionMode_Plus);
        for (const SparkParticle& s : m_hitSparks) {
            if (!s.isDebris && m_stage != 1) { // 剔除虫族
                QPointF currentPos = s.direction * (s.speed * m_hitEffectProgress);
                QColor c = s.color;
                c.setAlphaF(alphaFade);
                painter->setBrush(c);
                painter->setPen(Qt::NoPen);

                painter->save();
                painter->translate(currentPos);
                float angle = std::atan2(s.direction.y(), s.direction.x()) * 180.0 / M_PI;
                painter->rotate(angle);
                float sparkLength = s.size * 5.0f * (1.0f - m_hitEffectProgress);
                painter->drawEllipse(QRectF(-sparkLength, -s.size / 2, sparkLength * 2, s.size));
                painter->restore();
            }
        }

        // 【第三层：外星战舰专属战术护盾】
        if (m_stage == 0 && !m_isFatalExploding) {
            float shieldRadius = 65.0f + 15.0f * m_hitEffectProgress;
            QPolygonF hexagon;
            for (int i = 0; i < 6; ++i) {
                qreal angle = i * M_PI / 3.0;
                hexagon << QPointF(shieldRadius * std::cos(angle), shieldRadius * std::sin(angle));
            }
            painter->setPen(QPen(QColor(0, 200, 255, 180 * alphaFade), 3.0f));
            painter->setBrush(QColor(0, 100, 255, 40 * alphaFade));
            painter->drawPolygon(hexagon);
        }

        // 【第四层：致命命中专属的阵营能量过载特效】
        if (m_isFatalExploding) {
            float currentRadius = m_hitEffectProgress * 120.0f;
            QColor ringColor;
            QColor coreColor;

            if (m_stage == 2) {
                ringColor = QColor(255, 100, 0, 255 * alphaFade);
                coreColor = QColor(255, 200, 100, 200 * alphaFade);
            }
            else if (m_stage == 1) {
                // 虫巢死亡时，爆发一圈绿色的毒气环，核心是令人作呕的暗血色肉囊
                ringColor = QColor(80, 255, 80, 150 * alphaFade);
                coreColor = QColor(80, 15, 30, 240 * alphaFade);
                currentRadius *= 0.8f; // 毒气环扩散比冲击波小一点，更浑厚
            }
            else {
                ringColor = QColor(0, 200, 255, 255 * alphaFade);
                coreColor = QColor(255, 255, 255, 200 * alphaFade);
            }

            QPen ringPen(ringColor);
            ringPen.setWidthF(m_stage == 1 ? 15.0f * alphaFade : 6.0f * alphaFade); // 虫族毒气环更粗更浑浊
            painter->setPen(ringPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(QPointF(0, 0), currentRadius, currentRadius);

            float coreRadius = 50.0f * (1.0f - m_hitEffectProgress);
            painter->setBrush(coreColor);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);
        }

        painter->restore();
    }

}

// 【修改】实现带伤害来源的 takeDamage
bool PlaneEntity::takeDamage(float dmg, PlayerEntity* player)
{
    if (!isAlive() || property("hitFrozen").toBool()) return false;
    m_hp -= dmg;
    if (m_hp <= 0.0f) {
        m_hp = 0.0f;
        // 【核心修改】：敌机阵亡时，主动调用玩家身上的加分接口
        if (player) {
            player->addScore(m_scoreValue);
            
        }
        return true; // 阵亡
    }
    return false; // 受伤但未阵亡
}

void PlaneEntity::startSelectedAnimation()
{
    // 重置并保证受击时的初始基础状态
    setOpacity(1.0);
    setFlash(0.0);

    // ================= 【区分阵营：生成毁灭级过载粒子】 =================
    m_isFatalExploding = true;
    m_hitSparks.clear();
    setHitEffectProgress(0.0);

    // 虫族死亡爆浆需要海量粒子来支撑血腥感
    int debrisCount = (m_stage == 1) ? (30 + QRandomGenerator::global()->bounded(20))
        : (15 + QRandomGenerator::global()->bounded(10));

    for (int i = 0; i < debrisCount; ++i) {
        SparkParticle s;
        float angle = QRandomGenerator::global()->generateDouble() * M_PI * 2.0;
        s.direction = QPointF(std::cos(angle), std::sin(angle));
        s.speed = 60.0f + QRandomGenerator::global()->generateDouble() * 120.0f;
        s.size = 5.0f + QRandomGenerator::global()->generateDouble() * 10.0f;

        if (m_stage == 2) {
            s.isDebris = QRandomGenerator::global()->generateDouble() > 0.4;
            s.color = s.isDebris ? QColor(40 + QRandomGenerator::global()->bounded(30), 40, 40) : QColor(255, 150 + QRandomGenerator::global()->bounded(100), 0);
        }
        else if (m_stage == 1) { // 虫巢：令人极度不适的血肉爆裂
            s.isDebris = QRandomGenerator::global()->generateDouble() > 0.6;
            if (s.isDebris) {
                // 巨大的暗红/黑紫色脏器肉块
                s.color = QRandomGenerator::global()->generateDouble() > 0.5 ? QColor(90, 10, 30) : QColor(60, 15, 60);
                s.size = 6.0f + QRandomGenerator::global()->generateDouble() * 14.0f; // 肉块体积巨大
                s.speed = 30.0f + QRandomGenerator::global()->generateDouble() * 60.0f; // 沉重，飞得慢
            }
            else {
                // 大量呈高压喷射状的毒液和体液
                s.color = QRandomGenerator::global()->generateDouble() > 0.3 ? QColor(100, 255, 50) : QColor(30, 100, 20);
                s.size = 3.0f + QRandomGenerator::global()->generateDouble() * 6.0f;
                s.speed = 100.0f + QRandomGenerator::global()->generateDouble() * 180.0f; // 液体爆射
            }
        }
        else {
            s.isDebris = false;
            s.color = QColor(0, 200 + QRandomGenerator::global()->bounded(55), 255);
        }
        m_hitSparks.append(s);
    }
    // =================================================================

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // 1. 锐利爆闪 (瞬间极白，然后快速衰减，模拟爆炸闪光)
    QPropertyAnimation* flashAnim = new QPropertyAnimation(this, "flash", this);
    flashAnim->setDuration(200);
    flashAnim->setKeyValueAt(0.0, 0.0);
    flashAnim->setKeyValueAt(0.1, 1.0); // 10%的时间瞬间达到最亮
    flashAnim->setKeyValueAt(0.5, 0.3); // 快速散去
    flashAnim->setKeyValueAt(1.0, 0.0);
    group->addAnimation(flashAnim);

    // 2. 狂暴物理震颤 (大幅度无规律位移，模拟极强动能冲击)
    QPropertyAnimation* shakeAnim = new QPropertyAnimation(this, "pos", this);
    shakeAnim->setDuration(400);
    QPointF originPos = this->pos();
    shakeAnim->setKeyValueAt(0.0, originPos);
    shakeAnim->setKeyValueAt(0.1, originPos + QPointF(-18, -12)); // 初始冲击极强
    shakeAnim->setKeyValueAt(0.2, originPos + QPointF(15, 18));
    shakeAnim->setKeyValueAt(0.3, originPos + QPointF(-20, 15));
    shakeAnim->setKeyValueAt(0.4, originPos + QPointF(18, -16));
    shakeAnim->setKeyValueAt(0.5, originPos + QPointF(-12, -10));
    shakeAnim->setKeyValueAt(0.6, originPos + QPointF(10, 12));
    shakeAnim->setKeyValueAt(0.8, originPos + QPointF(-5, 6));    // 快速衰减
    shakeAnim->setKeyValueAt(1.0, originPos); // 准时归位迎接爆炸
    group->addAnimation(shakeAnim);

    // 3. 结构顿挫 (向内瞬间压缩，不产生回弹的形变)
    QPropertyAnimation* scaleXAnim = new QPropertyAnimation(this, "scaleX", this);
    scaleXAnim->setDuration(400);
    scaleXAnim->setKeyValueAt(0.0, 1.0);
    scaleXAnim->setKeyValueAt(0.15, 0.85); // 瞬间被物理冲击向内挤压
    scaleXAnim->setKeyValueAt(1.0, 0.95);  // 保持一定的受损形变，不再像皮球一样弹回
    group->addAnimation(scaleXAnim);

    QPropertyAnimation* scaleYAnim = new QPropertyAnimation(this, "scaleY", this);
    scaleYAnim->setDuration(400);
    scaleYAnim->setKeyValueAt(0.0, 1.0);
    scaleYAnim->setKeyValueAt(0.15, 0.90);
    scaleYAnim->setKeyValueAt(1.0, 0.95);
    group->addAnimation(scaleYAnim);

    // ================= 【新增：过载特效进度驱动】 =================
    QPropertyAnimation* overloadAnim = new QPropertyAnimation(this, "hitEffectProgress", this);
    overloadAnim->setDuration(400); // 与你原有的 shakeAnim 400ms 保持绝对同步
    overloadAnim->setStartValue(0.0);
    overloadAnim->setEndValue(1.0);
    overloadAnim->setEasingCurve(QEasingCurve::OutExpo); // 极其暴力的减速曲线
    group->addAnimation(overloadAnim);
    // ===============================================================

    // 监听动画结束，回调给 Controller 执行真正的爆炸和销毁逻辑
    connect(group, &QParallelAnimationGroup::finished, this, &GameEntity::selectedAnimationFinished);

    // 启动动画组，并在结束后自动释放内存
    group->start(QAbstractAnimation::DeleteWhenStopped);

}

void PlaneEntity::startHitAnimation()
{
	m_isFatalExploding = false; // 这只是受击动画，不是阵亡爆炸，重置这个状态以防万一
    // 强制终止当前可能正在播放的同类动画
    setScaleX(1.0);
    setScaleY(1.0);
    setFlash(0.0);

    // ================= 【区分阵营：生成受击特效粒子】 =================
    m_hitSparks.clear();
    // 虫族受击时，血液更容易大量飞溅，粒子数翻倍
    int sparkCount = (m_stage == 1) ? (15 + QRandomGenerator::global()->bounded(10))
        : (8 + QRandomGenerator::global()->bounded(6));

    for (int i = 0; i < sparkCount; ++i) {
        SparkParticle s;
        float angle = QRandomGenerator::global()->generateDouble() * M_PI * 2.0;
        s.direction = QPointF(std::cos(angle), std::sin(angle));
        s.speed = 50.0f + QRandomGenerator::global()->generateDouble() * 80.0f;
        s.size = 2.0f + QRandomGenerator::global()->generateDouble() * 4.0f;

        if (m_stage == 2) { // 军方飞机：溅射火花与金属装甲碎片
            s.isDebris = QRandomGenerator::global()->generateDouble() > 0.7;
            s.color = s.isDebris ? QColor(150, 160, 170) : QColor(255, 180 + QRandomGenerator::global()->bounded(75), 50);
        }
        else if (m_stage == 1) { // 虫族：血腥飙血
            s.isDebris = QRandomGenerator::global()->generateDouble() > 0.8; // 20%是碎肉，80%是血液
            if (s.isDebris) {
                // 暗紫/深红色的残肢碎肉
                s.color = QColor(80 + QRandomGenerator::global()->bounded(40), 20, 60);
                s.size = 3.0f + QRandomGenerator::global()->generateDouble() * 4.0f;
            }
            else {
                // 浓稠的深浅不一的绿血
                s.color = QColor(20 + QRandomGenerator::global()->bounded(30), 120 + QRandomGenerator::global()->bounded(100), 20);
                s.size = 2.0f + QRandomGenerator::global()->generateDouble() * 5.0f; // 血液大小颗粒分明
                s.speed = 80.0f + QRandomGenerator::global()->generateDouble() * 120.0f; // 血液受压喷射速度极快
            }
        }
        else { // 外星战舰：蓝色能量泄漏碎片
            s.isDebris = false;
            s.color = QColor(0, 200 + QRandomGenerator::global()->bounded(55), 255);
        }
        m_hitSparks.append(s);
    }
    // =================================================================

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // [保留原有的闪白动画 flashAnim]
    QPropertyAnimation* flashAnim = new QPropertyAnimation(this, "flash", this);
    flashAnim->setDuration(120);
    flashAnim->setKeyValueAt(0.0, 0.0);
    flashAnim->setKeyValueAt(0.3, 0.9);
    flashAnim->setKeyValueAt(1.0, 0.0);
    group->addAnimation(flashAnim);

    // [保留原有的形变动画 scaleXAnim 和 scaleYAnim]
    QPropertyAnimation* scaleXAnim = new QPropertyAnimation(this, "scaleX", this);
    scaleXAnim->setDuration(120);
    scaleXAnim->setKeyValueAt(0.0, 1.0);
    scaleXAnim->setKeyValueAt(0.3, 1.15);
    scaleXAnim->setKeyValueAt(1.0, 1.0);
    group->addAnimation(scaleXAnim);

    QPropertyAnimation* scaleYAnim = new QPropertyAnimation(this, "scaleY", this);
    scaleYAnim->setDuration(120);
    scaleYAnim->setKeyValueAt(0.0, 1.0);
    scaleYAnim->setKeyValueAt(0.3, 0.85);
    scaleYAnim->setKeyValueAt(1.0, 1.0);
    group->addAnimation(scaleYAnim);

    // ================= 【新增：粒子扩散进度动画】 =================
    QPropertyAnimation* sparksAnim = new QPropertyAnimation(this, "hitEffectProgress", this);
    sparksAnim->setDuration(400); // 火花留存时间比形变稍微长一点，与导弹特效对齐
    sparksAnim->setStartValue(0.0);
    sparksAnim->setEndValue(1.0);
    // OutCubic 曲线：爆炸初速度极快，随后受到空气阻力迅速减速飘散
    sparksAnim->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(sparksAnim);
    // =============================================================

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void PlaneEntity::setHitEffectProgress(qreal progress)
{
    if (qFuzzyCompare(m_hitEffectProgress, progress)) return;
    m_hitEffectProgress = progress;
    update(); // 进度变化时触发重绘
    emit hitEffectProgressChanged();

}
