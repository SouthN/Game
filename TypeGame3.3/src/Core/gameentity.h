/* ------------------------------------------------------------------
// 文件名     : gameentity.h
// 创建者     : [你的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 游戏实体基类，彻底封装 2D 渲染图元、属性动画和物理运动逻辑
------------------------------------------------------------------ */
#pragma once
#include <QGraphicsObject>
#include <QPainter>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QPauseAnimation>
#include <QRectF>

class GameEntity : public QGraphicsObject
{
    Q_OBJECT
    // 注册所有共用的属性动画
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(qreal scaleX READ scaleX WRITE setScaleX NOTIFY scaleXChanged)
    Q_PROPERTY(qreal scaleY READ scaleY WRITE setScaleY NOTIFY scaleYChanged)
    Q_PROPERTY(qreal flash READ flash WRITE setFlash NOTIFY flashChanged)

public:
    GameEntity();
    ~GameEntity() override = default;

    // ========= 禁用拷贝和赋值 =========
    GameEntity(const GameEntity&) = delete;
    GameEntity& operator=(const GameEntity&) = delete;

    // 纯虚函数：子类必须实现具体的贴图绘制逻辑和内存池回收逻辑
    virtual void paint(QPainter* painter) = 0;
    virtual void destroy() = 0; 

    // QGraphicsItem 接口基类兜底实现
	QRectF boundingRect() const override { return m_rect; } // 默认包围盒为 m_rect，子类可通过 setBoundingRect 调整
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        this->paint(painter);
    }

	// 更新运动逻辑，默认实现为简单的下落，子类可重写实现更复杂的行为
    virtual void updateLogic(float deltaTime = 0.0f); 

    // 核心属性 Getter & Setter
    QChar getLetter() const { return m_letter; }
    void setLetter(QChar letter) { m_letter = letter; }
    
    void setFallSpeed(float speed) { m_fallSpeed = speed; }
    float getFallSpeed() const { return m_fallSpeed; }

    bool isAlive() const { return m_isAlive; }
    void setAlive(bool alive) { m_isAlive = alive; }
    
	bool isSelected() const { return m_isSelected; } // 选中状态，触发特殊动画效果
    void setSelected(bool selected);
    
    void setBoundingRect(const QRectF& rect) { m_rect = rect; }

    // ================= 动画接口 =================
    qreal opacity() const;
    void setOpacity(qreal opacity);
    void startFadeIn(int duration = 300);
    void startFadeOut(int duration = 300);

    // 【修改点】：加上 virtual 关键字，允许子类自定义打击动画
    virtual void startSelectedAnimation(); 

    qreal scaleX() const;
    void setScaleX(qreal scaleX);
    
    qreal scaleY() const;
    void setScaleY(qreal scaleY);
    
    qreal flash() const;
    void setFlash(qreal flash);

signals:
    void opacityChanged();
    void fadeOutFinished();
    void selectedAnimationFinished();
    void scaleXChanged();
    void scaleYChanged();
    void flashChanged();

protected:
    QRectF m_rect;
    bool m_isAlive = true;
    QChar m_letter;
    float m_fallSpeed = 1.0f;
	bool m_isSelected = false; // 选中状态，触发特殊动画效果

    // 动画状态与对象
    qreal m_opacity = 1.0;
    QPropertyAnimation* m_fadeInAnim = nullptr;
    QPropertyAnimation* m_fadeOutAnim = nullptr;

    qreal m_scaleX = 1.0;
    qreal m_scaleY = 1.0;
    qreal m_flash = 0.0;
};