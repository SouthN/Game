#include "bossentity.h"
#include "resourcemanager.h"
#include "entityfactory.h"
#include <QPropertyAnimation>
#include <QRandomGenerator> 
#include <QSizeF> // 这个头文件包含了 QSizeF 类的定义，用于处理尺寸相关的计算
#include <QtMath>

namespace {

    // 将“原始素材图片像素坐标”转换成 BossEntity 内部使用的局部坐标。
    // BossEntity 的判定坐标以 Boss 中心为 (0,0)，而贴图绘制时会 KeepAspectRatio
    // 缩放后居中显示在 BOSS_WIDTH x BOSS_HEIGHT 的 boundingRect 中。
    QRectF bossPixelRectToLocal(const QRectF& pixelRect, const QSizeF& sourceSize)
    {
        const qreal scale = qMin(
            BossEntity::BOSS_WIDTH / sourceSize.width(),
            BossEntity::BOSS_HEIGHT / sourceSize.height()
        );

        const QSizeF scaledTextureSize(sourceSize.width() * scale, sourceSize.height() * scale);
        const QPointF textureTopLeft(
            (BossEntity::BOSS_WIDTH - scaledTextureSize.width()) / 2.0,
            (BossEntity::BOSS_HEIGHT - scaledTextureSize.height()) / 2.0
        );
        const QPointF bossCenter(BossEntity::BOSS_WIDTH / 2.0, BossEntity::BOSS_HEIGHT / 2.0);

        return QRectF(
            textureTopLeft.x() + pixelRect.x() * scale - bossCenter.x(),
            textureTopLeft.y() + pixelRect.y() * scale - bossCenter.y(),
            pixelRect.width() * scale,
            pixelRect.height() * scale
        );
    }

    // 输入格式为左上角/右下角像素坐标：[x1, y1, x2, y2]，右下角为包含式坐标。
    QRectF pixelBox(int x1, int y1, int x2, int y2)
    {
        return QRectF(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    }

} // namespace


BossEntity::BossEntity()
{
    m_rect = QRectF(0, 0, BOSS_WIDTH, BOSS_HEIGHT);
    setBoundingRect(m_rect);
	setTransformOriginPoint(BOSS_WIDTH / 2.0, BOSS_HEIGHT / 2.0); // 设置变换中心为Boss中心，方便后续的动画效果
}

void BossEntity::initBoss(int type, qreal sceneWidth)
{
    m_bossType = type;
    m_sceneWidth = sceneWidth;
    setAlive(true);
    setOpacity(1.0);
    setScale(1.0);
    setFlash(0.0);

    // 清空并初始化多部位配置
    m_parts.clear();
    // 下面的矩形均使用你标注图中的原始像素坐标 [x1, y1, x2, y2]，
    // 再通过 bossPixelRectToLocal() 自动转换为 BossEntity 所需的“相对 Boss 中心”坐标。
    // 由于 paint() 中 Boss 贴图是 KeepAspectRatio 后居中绘制的，这里也使用完全一致的缩放/居中逻辑。
    if (type == 0) {
        const QSizeF sourceSize(635, 619);

        m_parts.append({ 0, "Left Wing", bossPixelRectToLocal(pixelBox(1, 55, 268, 577), sourceSize), 30.0f, 30.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 1, "Right Wing", bossPixelRectToLocal(pixelBox(372, 57, 634, 578), sourceSize), 30.0f, 30.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 2, "Core Eye", bossPixelRectToLocal(pixelBox(266, 74, 370, 177), sourceSize), 40.0f, 40.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 3, "Main Hull", bossPixelRectToLocal(pixelBox(266, 176, 374, 612), sourceSize), 50.0f, 50.0f, false, false, QChar(), false, 0, 0.0 });
    }
    else if (type == 1) {
        const QSizeF sourceSize(627, 627);

        m_parts.append({ 0, "Left Blade", bossPixelRectToLocal(pixelBox(0, 40, 97, 612), sourceSize), 40.0f, 40.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 1, "Right Blade", bossPixelRectToLocal(pixelBox(532, 39, 620, 612), sourceSize), 40.0f, 40.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 2, "Brain Core", bossPixelRectToLocal(pixelBox(179, 155, 443, 471), sourceSize), 60.0f, 60.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 3, "Lower Hive", bossPixelRectToLocal(pixelBox(94, 470, 534, 625), sourceSize), 50.0f, 50.0f, false, false, QChar(), false, 0, 0.0 });
    }
    else {
        const QSizeF sourceSize(647, 763);

        // Boss3：侧边框与中央长框边缘有少量重叠，先加入四个侧边部件，再加入核心长框。
        m_parts.append({ 0, "Left Upper Generator", bossPixelRectToLocal(pixelBox(0, 44, 201, 357), sourceSize), 20.0f, 20.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 1, "Right Upper Generator", bossPixelRectToLocal(pixelBox(443, 43, 645, 350), sourceSize), 20.0f, 20.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 2, "Left Lower Generator", bossPixelRectToLocal(pixelBox(0, 354, 201, 756), sourceSize), 20.0f, 20.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 3, "Right Lower Generator", bossPixelRectToLocal(pixelBox(443, 348, 645, 755), sourceSize), 20.0f, 20.0f, false, false, QChar(), false, 0, 0.0 });
        m_parts.append({ 4, "Core Reactor", bossPixelRectToLocal(pixelBox(198, 4, 446, 726), sourceSize), 80.0f, 80.0f, false, false, QChar(), false, 0, 0.0 });
    }
    QString imgPath;
    QString damagedImgPath; // 新增：战损图路径
    // 战损图的文件名是在原名后加 "_b"
    if (type == 0) {
        imgPath = ":/SpaceWar/assets/images/SpaceWar/Boss1.png";
        damagedImgPath = ":/SpaceWar/assets/images/SpaceWar/Boss1_b.png";
    }
    else if (type == 1) {
        imgPath = ":/SpaceWar/assets/images/SpaceWar/Boss2.png";
        damagedImgPath = ":/SpaceWar/assets/images/SpaceWar/Boss2_b.png";
    }
    else {
        imgPath = ":/SpaceWar/assets/images/SpaceWar/Boss3.png";
        damagedImgPath = ":/SpaceWar/assets/images/SpaceWar/Boss3_b.png";
    }

    // 1. 加载完整贴图并生成受击白闪贴图
    QPixmap pixmap = ResourceManager::instance()->loadPixmap(imgPath);
    if (!pixmap.isNull()) {
        m_texture = pixmap.scaled(BOSS_WIDTH, BOSS_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPixmap flashFrame(m_texture.size());
        flashFrame.fill(Qt::transparent);
        QPainter p(&flashFrame);
        p.drawPixmap(0, 0, m_texture);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(flashFrame.rect(), Qt::white);
        p.end();
        m_flashTexture = flashFrame;
    }

    // 2. 【第一步新增】：加载对应的战损版贴图，使用绝对一致的缩放策略
    QPixmap damagedPixmap = ResourceManager::instance()->loadPixmap(damagedImgPath);
    if (!damagedPixmap.isNull()) {
        m_damagedTexture = damagedPixmap.scaled(BOSS_WIDTH, BOSS_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (QRandomGenerator::global()->bounded(2) == 0) {
        m_vx = -std::abs(m_vx);
    }
    if (QRandomGenerator::global()->bounded(2) == 0) {
        m_vx = -std::abs(m_vx);
    }

    // ================== 【新增：加载对应的爆炸序列帧】 ==================
    m_explosionFrames.clear();
    QString expPath;
    if (type == 0) expPath = ":/SpaceWar/assets/images/SpaceWar/Explosion5.png";      // 地球军/普通
    else if (type == 1) expPath = ":/SpaceWar/assets/images/SpaceWar/Explosion6.png"; // 异虫
    else expPath = ":/SpaceWar/assets/images/SpaceWar/Explosion4.png";                // 泽塔星系

    QPixmap sheet = ResourceManager::instance()->loadPixmap(expPath);
    if (!sheet.isNull()) {
        int cols = 5; int rows = 5;
        int frameW = sheet.width() / cols;
        int frameH = sheet.height() / rows;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                m_explosionFrames.append(sheet.copy(x * frameW, y * frameH, frameW, frameH));
            }
        }
    }
}

// 新增：替代原有的 takeDamage，实现精准部位打击
BossPart* BossEntity::checkAndDamagePart(const QPointF& localPos, float dmg)
{
    if (!isAlive()) return nullptr;

    // 将原点从左上角转化为相对于 Boss 中心点
    qreal cx = BOSS_WIDTH / 2.0;
    qreal cy = BOSS_HEIGHT / 2.0;
    QPointF relativePos(localPos.x() - cx, localPos.y() - cy);

    for (int i = 0; i < m_parts.size(); ++i) {
        BossPart& part = m_parts[i];

        // 如果该部位已经被飞弹彻底摧毁，或者子弹坐标不在该部位的矩形内，则跳过
        if (part.isDestroyed || !part.localRect.contains(relativePos)) {
            continue;
        }

        // 如果部位还没被破甲，机枪子弹可以造成伤害
        if (!part.isExposed) {
            part.currentHp -= dmg;
            if (part.currentHp <= 0.0f) {
                part.currentHp = 0.0f;
                part.isExposed = true; // 破甲完成，等待分配字母
            }
        }

        // 返回被击中的部位指针
        return &m_parts[i];
    }

    return nullptr; // 没有击中任何有效部位
}

void BossEntity::updateLogic(float deltaTime)
{
    if (!isAlive()) return;
    prepareGeometryChange();

    // ================== 【新增：濒死连环爆炸逻辑】 ==================
    if (m_isDying) {
        m_deathTimer += deltaTime;
        m_explosionEffectTimer += deltaTime;

        // 每 0.3 秒在 Boss 身体上随机产生一次爆炸火花
        if (m_explosionEffectTimer >= 0.3f) {
            m_explosionEffectTimer = 0.0f;

            // 随机生成一个相对于 Boss 左上角的局部坐标 (限制在 80% 核心区域内)
            float rx = (QRandomGenerator::global()->generateDouble() * 0.8 + 0.1) * BOSS_WIDTH;
            float ry = (QRandomGenerator::global()->generateDouble() * 0.8 + 0.1) * BOSS_HEIGHT;

            startHitAnimation(QPointF(rx, ry));
            emit chainExplosionTriggered(); // 通知 Controller 播放音效
        }

        // 3 秒后结束连环爆炸，触发最终的死亡结算
        if (m_deathTimer >= 3.0f) {
            m_isDying = false;
            emit deathSequenceFinished();
        }
        return; // 濒死状态下，强行停止 Boss 的移动
    }

	// Boss 移动逻辑：先垂直向下进入屏幕，达到指定位置后开始水平来回移动
    // 如果还没完全进入屏幕指定位置，先垂直向下缓慢移动，屏蔽左右移动
    if (y() < 0.0) {
        setPos(x(), y() + m_vy * deltaTime);
        return;
    }
    // 仅在水平方向上移动
    qreal nextX = x() + m_vx * deltaTime;
    // 触碰屏幕边缘反弹
    if (nextX < 0) {
        nextX = 0;
        m_vx = std::abs(m_vx);
    }
    else if (nextX + BOSS_WIDTH > m_sceneWidth) {
        nextX = m_sceneWidth - BOSS_WIDTH;
        m_vx = -std::abs(m_vx);
    }
    setPos(nextX, y());

    // ================== 【新增：触发攻击系统】 ==================
    // 只有 Boss 完全进场后，才开始计算攻击和弹幕
    processAttack(deltaTime);
}


bool BossEntity::areAllPartsDestroyed() const
{
    for (const BossPart& part : m_parts) {
        if (!part.isDestroyed) {
            return false;
        }
    }
    return true;
}

void BossEntity::startHitAnimation(const QPointF& localHitPos, BossPart* hitPart)
{
    m_isFatalExploding = false;
    setScaleX(1.0);
    setScaleY(1.0);
    setFlash(0.0);
    // 如果没有指定受击部位，则重置全局闪光（用于濒死大爆炸）
    if (!hitPart) {
        setFlash(0.0);
    }

    m_hitSparks.clear();

    int sparkCount = 20;
    QColor sparkColor = QColor(255, 200, 50); // 默认金属火花颜色

    // 将局部击中点转换为相对于 Boss 绘图中心 (0,0) 的偏移量
    // （在 initBoss 时，如果 Boss 的尺寸是 450x400，它的中心点 cx, cy 就是 225, 200）
    QPointF centerOffset(localHitPos.x() - BOSS_WIDTH / 2.0, localHitPos.y() - BOSS_HEIGHT / 2.0);

    for (int i = 0; i < sparkCount; ++i) {
        SparkParticle s;
        float angle = QRandomGenerator::global()->generateDouble() * M_PI * 2.0;
        s.direction = QPointF(std::cos(angle), std::sin(angle));
        s.speed = 100.0f + QRandomGenerator::global()->generateDouble() * 150.0f;
        s.size = 3.0f + QRandomGenerator::global()->generateDouble() * 6.0f;
        s.color = sparkColor;
        s.isDebris = (QRandomGenerator::global()->generateDouble() > 0.6);
        s.startPos = centerOffset; // 精准设置火花的喷发源头！

        m_hitSparks.append(s);
    }

    if (hitPart) {
        // 【新增】：每次受击时强制归零，防止连续命中时的叠加闪烁
        hitPart->flashOpacity = 0.0;

        QVariantAnimation* flashAnim = new QVariantAnimation(this);
        flashAnim->setDuration(150);
        flashAnim->setKeyValueAt(0.0, 0.0);
        flashAnim->setKeyValueAt(0.3, 0.5);
        flashAnim->setKeyValueAt(1.0, 0.0);

        int partId = hitPart->id;
        connect(flashAnim, &QVariantAnimation::valueChanged, this, [this, partId](const QVariant& value) {
            for (int i = 0; i < m_parts.size(); ++i) {
                if (m_parts[i].id == partId) {
                    m_parts[i].flashOpacity = value.toReal();
                    update(); // 触发重绘
                    break;
                }
            }
            });
        // 【修改】：直接让动画独立启动并自我销毁
        flashAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }
    else {
        QPropertyAnimation* flashAnim = new QPropertyAnimation(this, "flash", this);
        flashAnim->setDuration(150);
        flashAnim->setKeyValueAt(0.0, 0.0);
        flashAnim->setKeyValueAt(0.3, 0.5);
        flashAnim->setKeyValueAt(1.0, 0.0);
        // 【修改】：直接独立启动
        flashAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 火花四溅的物理衰减进度（保持不变）
    QPropertyAnimation* sparksAnim = new QPropertyAnimation(this, "hitEffectProgress", this);
    sparksAnim->setDuration(400);
    sparksAnim->setStartValue(0.0);
    sparksAnim->setEndValue(1.0);
    sparksAnim->setEasingCurve(QEasingCurve::OutCubic);
    // 【修改】：直接独立启动
    sparksAnim->start(QAbstractAnimation::DeleteWhenStopped);

}

void BossEntity::startSelectedAnimation()
{
    setOpacity(1.0);
    setFlash(0.0);
    m_isFatalExploding = true;
    m_hitSparks.clear();
    setHitEffectProgress(0.0);

    // 毁灭级粒子爆发 (数量庞大)
    int debrisCount = (m_bossType == 1) ? (80 + QRandomGenerator::global()->bounded(40))
        : (50 + QRandomGenerator::global()->bounded(20));

    for (int i = 0; i < debrisCount; ++i) {
        SparkParticle s;
        float angle = QRandomGenerator::global()->generateDouble() * M_PI * 2.0;
        s.direction = QPointF(std::cos(angle), std::sin(angle));
        s.speed = 150.0f + QRandomGenerator::global()->generateDouble() * 250.0f;
        s.size = 8.0f + QRandomGenerator::global()->generateDouble() * 15.0f; // 碎片尺寸大幅增加

        if (m_bossType == 0) {
            s.isDebris = QRandomGenerator::global()->generateDouble() > 0.4;
            s.color = s.isDebris ? QColor(40 + QRandomGenerator::global()->bounded(30), 40, 40) : QColor(255, 150 + QRandomGenerator::global()->bounded(100), 0);
        }
        else if (m_bossType == 1) {
            s.isDebris = QRandomGenerator::global()->generateDouble() > 0.6;
            if (s.isDebris) {
                s.color = QRandomGenerator::global()->generateDouble() > 0.5 ? QColor(90, 10, 30) : QColor(60, 15, 60);
                s.size = 12.0f + QRandomGenerator::global()->generateDouble() * 25.0f; // 巨型肉块
                s.speed = 80.0f + QRandomGenerator::global()->generateDouble() * 100.0f;
            }
            else {
                s.color = QRandomGenerator::global()->generateDouble() > 0.3 ? QColor(100, 255, 50) : QColor(30, 100, 20);
                s.speed = 200.0f + QRandomGenerator::global()->generateDouble() * 300.0f;
            }
        }
        else {
            s.isDebris = false;
            s.color = QColor(0, 200 + QRandomGenerator::global()->bounded(55), 255);
        }
        m_hitSparks.append(s);
    }

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // 锐利爆闪
    QPropertyAnimation* flashAnim = new QPropertyAnimation(this, "flash", this);
    flashAnim->setDuration(300);
    flashAnim->setKeyValueAt(0.0, 0.0);
    flashAnim->setKeyValueAt(0.1, 1.0);
    flashAnim->setKeyValueAt(0.5, 0.4);
    flashAnim->setKeyValueAt(1.0, 0.0);
    group->addAnimation(flashAnim);

    // 史诗级狂暴物理震颤 (根据 Boss 尺寸大幅提高震动像素)
    QPropertyAnimation* shakeAnim = new QPropertyAnimation(this, "pos", this);
    shakeAnim->setDuration(600);
    QPointF originPos = this->pos();
    shakeAnim->setKeyValueAt(0.0, originPos);
    shakeAnim->setKeyValueAt(0.1, originPos + QPointF(-40, -30));
    shakeAnim->setKeyValueAt(0.2, originPos + QPointF(35, 40));
    shakeAnim->setKeyValueAt(0.3, originPos + QPointF(-45, 35));
    shakeAnim->setKeyValueAt(0.4, originPos + QPointF(40, -35));
    shakeAnim->setKeyValueAt(0.6, originPos + QPointF(-20, -15));
    shakeAnim->setKeyValueAt(0.8, originPos + QPointF(15, 20));
    shakeAnim->setKeyValueAt(1.0, originPos);
    group->addAnimation(shakeAnim);

    // 结构向内顿挫坍缩
    QPropertyAnimation* scaleXAnim = new QPropertyAnimation(this, "scaleX", this);
    scaleXAnim->setDuration(600);
    scaleXAnim->setKeyValueAt(0.0, 1.0);
    scaleXAnim->setKeyValueAt(0.15, 0.75); // 猛烈向内吸入
    scaleXAnim->setKeyValueAt(1.0, 0.90);
    group->addAnimation(scaleXAnim);

    QPropertyAnimation* scaleYAnim = new QPropertyAnimation(this, "scaleY", this);
    scaleYAnim->setDuration(600);
    scaleYAnim->setKeyValueAt(0.0, 1.0);
    scaleYAnim->setKeyValueAt(0.15, 0.80);
    scaleYAnim->setKeyValueAt(1.0, 0.90);
    group->addAnimation(scaleYAnim);

    // 过载爆炸特效驱动
    QPropertyAnimation* overloadAnim = new QPropertyAnimation(this, "hitEffectProgress", this);
    overloadAnim->setDuration(600);
    overloadAnim->setStartValue(0.0);
    overloadAnim->setEndValue(1.0);
    overloadAnim->setEasingCurve(QEasingCurve::OutExpo);
    group->addAnimation(overloadAnim);

    connect(group, &QParallelAnimationGroup::finished, this, &GameEntity::selectedAnimationFinished);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void BossEntity::playPartExplosion(BossPart* part)
{
    if (m_explosionFrames.isEmpty() || !part) return;

    // 1. 创建独立的动画控制器，父对象设为 BossEntity 以自动管理内存
    SpriteAnimation* anim = new SpriteAnimation(this);
    for (const QPixmap& frame : m_explosionFrames) {
        anim->addFrame(frame);
    }
    anim->setLoop(false);
    anim->setFrameInterval(50); // 50毫秒一帧

    // 2. 存入活跃列表
    ActiveExplosion inst;
    inst.localPos = part->localRect.center(); // 将局部坐标定位在部位中心
    inst.anim = anim;
    m_activeExplosions.append(inst);

    // 3. 帧更新时触发实体的高频重绘
    connect(anim, &SpriteAnimation::frameChanged, this, [this]() {
        this->update();
        });

    // 4. 播放完毕清理并剔除引用
    connect(anim, &SpriteAnimation::animationFinished, this, [this, anim]() {
        for (int i = 0; i < m_activeExplosions.size(); ++i) {
            if (m_activeExplosions[i].anim == anim) {
                m_activeExplosions.removeAt(i);
                break;
            }
        }
        anim->deleteLater();
        this->update();
        });

    anim->start();
}

void BossEntity::startDefeatedSequence()
{
	m_isDying = true; // 启动濒死状态，进入连环爆炸阶段
	m_deathTimer = 0.0f; // 连环爆炸计时器
	m_explosionEffectTimer = 0.0f; // 用于控制每次爆炸特效的触发时机
}

void BossEntity::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(m_opacity);

    painter->save();
    QRectF bounds = boundingRect();
    QPointF center = bounds.center();

    // 应用整体的缩放和中心平移
    painter->translate(center);
    painter->scale(m_scaleX, m_scaleY);
    painter->translate(-center);

    // ================= 【1. 绘制 Boss 完整本体贴图】 =================
    if (!m_texture.isNull()) {
        QPointF topLeft = bounds.center() - QPointF(m_texture.width() / 2.0, m_texture.height() / 2.0);
        painter->drawPixmap(topLeft, m_texture);
    }

    // ================= 【新增：1.5 绘制局部战损贴图】 =================
    // 如果战损贴图加载成功，则检查各部位状态
    if (!m_damagedTexture.isNull()) {
        QPointF topLeft = bounds.center() - QPointF(m_damagedTexture.width() / 2.0, m_damagedTexture.height() / 2.0);

        for (const BossPart& part : m_parts) {
            // 当部位被破甲露出字母 (isExposed) 或被彻底摧毁 (isDestroyed) 时，显示战损图
            if (part.isExposed || part.isDestroyed) {
                painter->save();
                // 核心机制：设置裁剪矩形。
                // 因为 bounds.center() 是实体中心，需要把它加到相对坐标的 localRect 上
                painter->setClipRect(part.localRect.translated(bounds.center()));

                // 画出战损版贴图，但因为有了 ClipRect，它只会显示在上述矩形框内
                painter->drawPixmap(topLeft, m_damagedTexture);
                painter->restore();
            }
        }
    }
    // =================================================================

    // 2. 局部 & 全局受击闪白叠加层
    if (!m_flashTexture.isNull()) {
        QPointF topLeft = bounds.center() - QPointF(m_flashTexture.width() / 2.0, m_flashTexture.height() / 2.0);

        // 2.1 优先绘制全局闪白（用于濒死连爆或毁灭大招）
        if (m_flash > 0.0) {
            painter->save();
            painter->setOpacity(m_flash);
            painter->drawPixmap(topLeft, m_flashTexture);
            painter->restore();
        }

        // 2.2 循环绘制各部位的局部闪白
        for (const BossPart& part : m_parts) {
            if (part.flashOpacity > 0.01) { // 剔除0.0不渲染，提升性能
                painter->save();
                painter->setOpacity(part.flashOpacity);
                // 【核心机制】：裁剪出部位所属的矩形区域，只在这个框内画白色剪影
                // 因为 bounds.center() 是实体中心，需要把它加到相对坐标的 localRect 上
                painter->setClipRect(part.localRect.translated(bounds.center()));
                painter->drawPixmap(topLeft, m_flashTexture);
                painter->restore();
            }
        }
    }

    // ================= 【新增：多部位状态独立渲染】 =================
    painter->save();
    painter->translate(bounds.center()); // 将原点移至 Boss 中心，与部位的 localRect 对齐

    // 获取系统时间用于做高频的呼吸/闪烁动画
    qint64 currentMs = QDateTime::currentMSecsSinceEpoch();
    qreal pulse = (std::sin(currentMs / 100.0) + 1.0) * 0.5; // 0.0 ~ 1.0 的快速脉冲

    for (const BossPart& part : m_parts) {
        QPointF partCenter = part.localRect.center();

        // 状态A：部位被彻底摧毁 -> 配合底层真实的战损贴图，仅叠加光效与烧焦阴影
        if (part.isDestroyed) {
            painter->save();
            painter->translate(partCenter);

            float craterSize = qMin(part.localRect.width(), part.localRect.height()) * 0.5f;

            // 1. 烧焦暗化层：利用“正片叠底”加深底层战损图的坑洞中心，使其更有立体深度
            painter->setCompositionMode(QPainter::CompositionMode_Multiply);
            QRadialGradient craterGrad(0, 0, craterSize);
            // 降低不透明度，让底层的 Boss_b.png 细节能够透过来
            craterGrad.setColorAt(0.0, QColor(30, 30, 30, 200));
            craterGrad.setColorAt(0.5, QColor(100, 100, 100, 100));
            craterGrad.setColorAt(1.0, QColor(255, 255, 255, 0)); // 边缘透明
            painter->setPen(Qt::NoPen);
            painter->setBrush(craterGrad);
            painter->drawEllipse(QPointF(0, 0), craterSize, craterSize);

            // 恢复默认混合模式，绘制少许装甲断层边缘线作为点缀
            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
            QPen edgePen(QColor(30, 30, 30, 150), 3, Qt::SolidLine, Qt::RoundCap);
            painter->setPen(edgePen);
            painter->setBrush(Qt::NoBrush);
            painter->drawArc(QRectF(-craterSize * 0.7, -craterSize * 0.7, craterSize * 1.4, craterSize * 1.4), 30 * 16, 120 * 16);

            // 2. 核心动态高亮：开启“加法混合”，绘制沸腾的能量泄漏/生物强酸
            // 加法混合可以确保能量光效十分通透刺眼，且绝不会遮盖底图的暗部细节
            painter->setCompositionMode(QPainter::CompositionMode_Plus);
            QColor coreColor, glowColor;
            if (m_bossType == 0) { // 军方：高温岩浆/明火
                coreColor = QColor(255, 100 + 50 * pulse, 0, 180);
                glowColor = QColor(255, 50, 0, 100 * pulse);
            }
            else if (m_bossType == 1) { // 虫巢：致命强酸毒液
                coreColor = QColor(50, 255, 50, 180);
                glowColor = QColor(20, 150, 20, 100 * pulse);
            }
            else { // 泽塔：电磁光晶坍缩
                coreColor = QColor(0, 200, 255, 180);
                glowColor = QColor(0, 100, 255, 100 * pulse);
            }

            QRadialGradient energyGrad(0, 0, craterSize * 0.6);
            energyGrad.setColorAt(0.0, coreColor);
            energyGrad.setColorAt(0.4, glowColor);
            energyGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
            painter->setBrush(energyGrad);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(0, 0), craterSize * 0.6, craterSize * 0.6);

            // 3. 表层：高频随机闪烁的短路电火花
            if ((currentMs + part.id * 100) % 500 < 50) {
                painter->setPen(QPen(coreColor.lighter(), 2));
                painter->drawLine(QPointF(-craterSize * 0.3, craterSize * 0.2), QPointF(craterSize * 0.4, -craterSize * 0.1));
                painter->drawLine(QPointF(0, 0), QPointF(craterSize * 0.2, craterSize * 0.4));
            }

            painter->restore();
        }
        // 状态B：被玩家敲击键盘锁定，导弹正在飞去 -> (保持不变，直接绘制在最上层)
        else if (part.isLocked) {
            painter->save();
            painter->translate(partCenter);

            qreal lockElapsed = (currentMs - part.lockTime) / 1000.0;
            qreal lockProgress = qMin(lockElapsed * 3.0, 1.0);
            qreal lockScale = 1.0 + (1.0 - lockProgress) * 1.5;

            painter->scale(lockScale, lockScale);
            painter->rotate(lockElapsed * 360.0);

            QColor warningColor(255, 50, 50, 220);
            QPen hexPen(warningColor, 2, Qt::DashLine);
            painter->setPen(hexPen);
            painter->setBrush(QColor(255, 0, 0, 40 * lockProgress));
            int r = 35;
            QPolygonF hexagon;
            for (int i = 0; i < 6; ++i) {
                qreal angle = i * M_PI / 3.0;
                hexagon << QPointF(r * std::cos(angle), r * std::sin(angle));
            }
            painter->drawPolygon(hexagon);

            painter->rotate(-lockElapsed * 720.0);
            painter->setPen(QPen(warningColor, 2));
            int tr = 18;
            QPolygonF triangle;
            for (int i = 0; i < 3; ++i) {
                qreal angle = i * 2.0 * M_PI / 3.0 - M_PI / 2.0;
                triangle << QPointF(tr * std::cos(angle), tr * std::sin(angle));
            }
            painter->drawPolygon(triangle);

            painter->restore();
        }
        // 状态C：破甲露出，等待玩家打字 -> 降低背景圆盘的不透明度，凸显底部的战损贴图
        else if (part.isExposed && !part.letter.isNull()) {
            painter->save();
            painter->translate(partCenter);

            float r = 28.0f;

            QColor frameColor(0, 255, 204, 100 + 100 * pulse);
            painter->setPen(QPen(frameColor, 2, Qt::SolidLine));
            // 【微调】：将背景圆盘变得更加透明 (从 150 降至 60)，让战损贴图透出来
            painter->setBrush(QColor(0, 40, 60, 60));

            painter->drawEllipse(QPointF(0, 0), r, r);
            painter->drawLine(QPointF(-r - 5, 0), QPointF(-r + 5, 0));
            painter->drawLine(QPointF(r - 5, 0), QPointF(r + 5, 0));
            painter->drawLine(QPointF(0, -r - 5), QPointF(0, -r + 5));
            painter->drawLine(QPointF(0, r - 5), QPointF(0, r + 5));

            QFont font("Consolas", 28, QFont::Bold, true);
            painter->setFont(font);

            painter->setPen(QColor(0, 0, 0, 255));
            painter->drawText(QRectF(-r, -r, 2 * r, 2 * r).translated(2, 2), Qt::AlignCenter, QString(part.letter));

            painter->setPen(QColor(0, 255, 204, 200 + 55 * pulse));
            painter->drawText(QRectF(-r, -r, 2 * r, 2 * r), Qt::AlignCenter, QString(part.letter));

            painter->restore();
        }
    }
    painter->restore(); // 结束 Boss 本体的 scale 和 translate

    // ================= 【新增：独立部位序列帧爆炸渲染】 =================
    if (!m_activeExplosions.isEmpty()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(bounds.center()); // 移入 Boss 局部坐标系

        // 开启加法混合，过滤纯黑背景，提亮火焰！
        painter->setCompositionMode(QPainter::CompositionMode_Plus);

        for (const ActiveExplosion& inst : m_activeExplosions) {
            QPixmap frame = inst.anim->getCurrentFrame();
            if (!frame.isNull()) {
                // 根据当前帧宽高，使爆炸以部位为中心完美对齐
                QRectF targetRect(inst.localPos.x() - frame.width() / 2.0,
                    inst.localPos.y() - frame.height() / 2.0,
                    frame.width(), frame.height());
                painter->drawPixmap(targetRect.toRect(), frame);
            }
        }
        painter->restore();
    }

    // ================= 【3. 双轨特效渲染层 (保留原代码不变)】 =================
    if (m_hitEffectProgress > 0.0 && m_hitEffectProgress < 1.0) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(bounds.center());

        float alphaFade = 1.0f - m_hitEffectProgress;

        // 【底层：实体碎片与虫族血浆】
        for (const SparkParticle& s : m_hitSparks) {
            if (s.isDebris || m_bossType == 1) {
                float vx = s.direction.x() * s.speed;
                float vy = s.direction.y() * s.speed;
                QPointF currentPos = s.startPos + QPointF(vx * m_hitEffectProgress, vy * m_hitEffectProgress);
                float dynamicAngle = 0.0f;

                if (m_bossType == 1) {
                    float gravityY = 400.0f * m_hitEffectProgress * m_hitEffectProgress;
                    currentPos.setY(currentPos.y() + gravityY);
                    float currentVy = vy + (800.0f * m_hitEffectProgress);
                    dynamicAngle = std::atan2(currentVy, vx) * 180.0 / M_PI;
                }

                QColor c = s.color;
                if (m_bossType == 1 && !s.isDebris) {
                    c = c.darker(100 + m_hitEffectProgress * 50);
                }
                c.setAlphaF(alphaFade);
                painter->setBrush(c);
                painter->setPen(Qt::NoPen);

                painter->save();
                painter->translate(currentPos);

                if (m_bossType == 1 && !s.isDebris) {
                    painter->rotate(dynamicAngle);
                    float stretch = 1.0f + (s.speed * 0.02f) * (1.0f - m_hitEffectProgress);
                    painter->drawEllipse(QRectF(-s.size * stretch, -s.size, s.size * stretch * 2.5f, s.size * 1.5f));
                }
                else {
                    painter->rotate(m_hitEffectProgress * (s.size > 12.0f ? 360.0 : 720.0));
                    painter->drawRoundedRect(-s.size, -s.size, s.size * 2, s.size * 2, s.size * 0.3, s.size * 0.3);
                }
                painter->restore();
            }
        }

        // 【中层：加法混合等离子光效】
        painter->setCompositionMode(QPainter::CompositionMode_Plus);
        for (const SparkParticle& s : m_hitSparks) {
            if (!s.isDebris && m_bossType != 1) {
                float vx = s.direction.x() * s.speed;
                float vy = s.direction.y() * s.speed;
                QPointF currentPos = s.startPos + QPointF(vx * m_hitEffectProgress, vy * m_hitEffectProgress);
                QColor c = s.color;
                c.setAlphaF(alphaFade);
                painter->setBrush(c);
                painter->setPen(Qt::NoPen);

                painter->save();
                painter->translate(currentPos);
                float angle = std::atan2(s.direction.y(), s.direction.x()) * 180.0 / M_PI;
                painter->rotate(angle);
                float sparkLength = s.size * 6.0f * (1.0f - m_hitEffectProgress);
                painter->drawEllipse(QRectF(-sparkLength, -s.size / 2, sparkLength * 2, s.size));
                painter->restore();
            }
        }

        // 【上层：致命阵亡专属的超级能量冲击波】
        if (m_isFatalExploding) {
            float currentRadius = m_hitEffectProgress * 400.0f;
            QColor ringColor, coreColor;

            if (m_bossType == 0) {
                ringColor = QColor(255, 100, 0, 255 * alphaFade);
                coreColor = QColor(255, 200, 100, 200 * alphaFade);
            }
            else if (m_bossType == 1) {
                ringColor = QColor(80, 255, 80, 150 * alphaFade);
                coreColor = QColor(80, 15, 30, 240 * alphaFade);
                currentRadius *= 0.85f;
            }
            else {
                ringColor = QColor(0, 200, 255, 255 * alphaFade);
                coreColor = QColor(255, 255, 255, 200 * alphaFade);
            }

            QPen ringPen(ringColor);
            ringPen.setWidthF(m_bossType == 1 ? 40.0f * alphaFade : 15.0f * alphaFade);
            painter->setPen(ringPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(QPointF(0, 0), currentRadius, currentRadius);

            float coreRadius = 150.0f * (1.0f - m_hitEffectProgress);
            painter->setBrush(coreColor);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(0, 0), coreRadius, coreRadius);
        }
        painter->restore();
    }
    painter->restore(); 
}

void BossEntity::setHitEffectProgress(qreal progress)
{
    // 使用 Qt 的安全浮点数比较，如果进度没变就直接返回，节省性能
    if (qFuzzyCompare(m_hitEffectProgress, progress)) return;

    m_hitEffectProgress = progress;

    // 进度更新后，触发图元重绘，让 paint 函数渲染最新的特效画面
    update();

    // 发送信号，通知属性动画当前进度已改变
    emit hitEffectProgressChanged();
}

void BossEntity::destroy()
{
    EntityFactory::instance()->destroyEntity<BossEntity>(this);
}

// ================== 【第二步新增：Boss 华丽弹幕算法】 ==================
void BossEntity::processAttack(float deltaTime)
{
    // 如果 Boss 正在濒死大爆炸，或者还没准备好，就不攻击
    if (m_isDying || m_parts.isEmpty()) return;

    m_attackTimer += deltaTime;
    if (m_attackTimer >= m_attackInterval) {
        m_attackTimer = 0.0f;
        m_attackPhase++; // 每次攻击阶段加1，用于交替技能

        // 计算 Boss 的世界中心坐标作为发射基准点
        QPointF spawnCenter = scenePos() + QPointF(BOSS_WIDTH / 2.0, BOSS_HEIGHT / 2.0);

        // ---------------------------------------------------------
        // Type 0: 地球军 Boss (重火力压制，扇形散弹交替)
        // ---------------------------------------------------------
        if (m_bossType == 0) {
            m_attackInterval = 2.0f; // 2秒一轮

            // 奇数轮发5发，偶数轮发7发，形成弹幕交错感
            int bulletCount = (m_attackPhase % 2 == 0) ? 7 : 5;
			float baseSpeed = 500.0f; // 军方的子弹飞行速度较快，增加压迫感

            // 扇形发射角度：正下方为 90 度，我们在 60 度到 120 度之间散开
            float startAngle = 60.0f;
            float endAngle = 120.0f;
            float angleStep = (endAngle - startAngle) / (bulletCount - 1);

            for (int i = 0; i < bulletCount; ++i) {
                float angleRad = (startAngle + i * angleStep) * M_PI / 180.0;
                QPointF vel(std::cos(angleRad) * baseSpeed, std::sin(angleRad) * baseSpeed);
                // 稍微偏下一点作为枪口
                emit fireBullet(0, spawnCenter + QPointF(0, 80), vel, 1.0f);
            }
        }
        // ---------------------------------------------------------
        // Type 1: 异虫巢穴 Boss (生物兵器，精准追踪与封锁)
        // ---------------------------------------------------------
        else if (m_bossType == 1) {
            m_attackInterval = 1.5f; // 虫族攻击频率快
            float speed = 350.0f;

            // 无论哪一阶段，都有一发精准朝向玩家当前位置飞去的毒液
            emit fireTargetedBullet(1, spawnCenter + QPointF(-40, 50), speed, 1.0f);
            emit fireTargetedBullet(1, spawnCenter + QPointF(0, 80), speed, 1.0f);
            emit fireTargetedBullet(1, spawnCenter + QPointF(-100, 20), speed, 1.0f);
            emit fireTargetedBullet(1, spawnCenter + QPointF(50, 35), speed, 1.0f);

            // 偶数轮追加额外的弧线/次级毒液，逼迫玩家走位
            if (m_attackPhase % 2 == 0) {
                emit fireTargetedBullet(1, spawnCenter + QPointF(40, 50), speed * 0.85f, 1.0f);
                emit fireTargetedBullet(1, spawnCenter + QPointF(0, 100), speed * 1.15f, 1.0f);
            }
        }
        // ---------------------------------------------------------
        // Type 2: 泽塔星系 Boss (高科技压迫感，几何阵列环形弹幕)
        // ---------------------------------------------------------
        else if (m_bossType == 2) {
            m_attackInterval = 2.5f; // 大招需要蓄力，频率稍慢
            float speed = 300.0f;

            if (m_attackPhase % 2 == 0) {
                // 大招：360度全屏绽放能量球
                int count = 18;
                for (int i = 0; i < count; ++i) {
                    float angleRad = (i * 360.0f / count) * M_PI / 180.0;
                    QPointF vel(std::cos(angleRad) * speed, std::sin(angleRad) * speed);
                    emit fireBullet(2, spawnCenter, vel, 1.0f);
                }
            }
            else {
                // 普攻：向下密集的锥形封锁线
                int count = 9;
                for (int i = 0; i < count; ++i) {
                    float angleRad = (50.0f + i * 10.0f) * M_PI / 180.0; // 50 到 130度
                    // 速度更快，逼迫玩家交出无敌或护盾
                    QPointF vel(std::cos(angleRad) * speed * 1.6f, std::sin(angleRad) * speed * 1.6f);
                    emit fireBullet(2, spawnCenter + QPointF(0, 100), vel, 1.0f);
                }
            }
        }
    }
}