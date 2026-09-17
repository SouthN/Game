/* ------------------------------------------------------------------
// 文件名     : playerentity.h
// 功能描述   : 玩家战机实体类，继承自 GameEntity。
//            负责加载 5 帧序列贴图，由控制器驱动进行姿态切换，并接管渲染。
//            【重构】：生命、能量、攻击力等数值已解耦至 PlayerAttributeComponent 组件。
------------------------------------------------------------------ */
#pragma once
#include "gameentity.h"
#include "playerattributecomponent.h" // 【新增】引入属性组件
#include <QVector>
#include <QPixmap>
#include <QString>


class PlayerEntity : public GameEntity
{
    Q_OBJECT
    // 新增：注册护盾动画进度属性
    Q_PROPERTY(qreal shieldProgress READ shieldProgress WRITE setShieldProgress NOTIFY shieldProgressChanged)

public:
    // 定义玩家战机的基础物理大小（可根据实际游戏手感微调）
    static constexpr int PLAYER_WIDTH = 100;
    static constexpr int PLAYER_HEIGHT = 130;

    explicit PlayerEntity();
    ~PlayerEntity() override;

    // 禁用拷贝和赋值
    PlayerEntity(const PlayerEntity&) = delete;
    PlayerEntity& operator=(const PlayerEntity&) = delete;

    // 实现基类纯虚函数
    void paint(QPainter* painter) override;
    void destroy() override;
    // 【新增】：重写 shape 方法以缩小实际碰撞面积
    QPainterPath shape() const override;


    // ================= 【新增：获取属性组件接口】 =================
    // 控制器和外部系统通过此接口访问/修改玩家的深度属性（如暴击率、机炮伤害）
    PlayerAttributeComponent* attributes() const { return m_attributes; }

    float getHp() const { return m_attributes->hp(); }
    float getMaxHp() const { return m_attributes->maxHp(); }
	void initHealth(float maxHp); // 初始化血量，设置当前血量为满血
    bool takeDamage(float dmg); // 返回 true 表示玩家坠毁
	void heal(float amount); // 命中奖励单词恢复血量，不能超过最大血量
    void addScore(int score); // 【新增】加分接口

    // 加载并切割横向序列帧素材
    void loadTexture(const QString& spriteSheetPath);
    
    // 接收控制器传入的移动方向，改变战机姿态
    // 参数dir 说明： -1 为左移，0 为不移动（居中平飞），1 为右移
    void setMovingDirection(int dir);

    // 重写基类的逻辑更新函数，处理平滑姿态过渡
    void updateLogic(float deltaTime = 0.0f) override;

    // 触发护盾动画
    void startShieldAnimation(); // 新增：触发护盾动画

    // 护盾进度的 Getter 和 Setter
    qreal shieldProgress() const { return m_shieldProgress; }
    void setShieldProgress(qreal progress);

    // 无敌帧状态接口
    bool isInvincible() const { return m_isInvincible; }
    void setInvincible(bool invincible) { m_isInvincible = invincible; }

signals:
    // 新增：护盾属性变化信号
    void shieldProgressChanged();

private:
    // 【核心新增】：挂载玩家的属性组件
    PlayerAttributeComponent* m_attributes = nullptr;

    QVector<QPixmap> m_frames;     // 存储切割后的 5 帧姿态贴图
    int m_currentFrameIndex = 2;   // 当前显示的帧索引（默认 2：平飞居中）
    int m_targetFrameIndex = 2;    // 目标帧索引（用于后续实现平滑过渡）

    // 【新增】：平滑过渡相关的控制变量
    float m_frameTimer = 0.0f;                             // 累加时间计时器
    static constexpr float FRAME_TRANSITION_DELAY = 0.05f; // 每帧过渡间隔时间 (50毫秒，可根据手感微调)

    qreal m_shieldProgress = 0.0;// 新增：护盾生命周期进度 (0.0 到 1.0)
    bool m_isInvincible = false; // 新增：无敌状态标志
};