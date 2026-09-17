/* ------------------------------------------------------------------
// 文件名     : playerattributecomponent.h
// 功能描述   : 玩家战机属性组件类。
//            集中管理玩家的生命、能量、攻击力、防御力、机动性等数值。
//            负责数值的安全校验和战斗相关的数值计算（暴击、护甲减伤等）。
------------------------------------------------------------------ */
#pragma once

#include <QObject>
#include <QtGlobal>


class PlayerAttributeComponent : public QObject
{
    Q_OBJECT
        // 注册属性，方便后续UI绑定、反射或动画系统调用
        Q_PROPERTY(float hp READ hp WRITE setHp NOTIFY hpChanged)
        Q_PROPERTY(float maxHp READ maxHp WRITE setMaxHp NOTIFY maxHpChanged)
        Q_PROPERTY(int energy READ energy WRITE setEnergy NOTIFY energyChanged)
        Q_PROPERTY(int maxEnergy READ maxEnergy WRITE setMaxEnergy NOTIFY maxEnergyChanged)

public:
    explicit PlayerAttributeComponent(QObject* parent = nullptr);
    ~PlayerAttributeComponent() override = default;

    // ================= 初始化与重置 =================
    void initDefaultStats(); // 初始化为默认数值

    // ================= 核心计算逻辑 =================
    // 处理受到的伤害，内部自动计算护甲减伤，返回实际扣除的血量
    float applyDamage(float rawDamage);
    // 恢复生命值，自动限制在 MaxHp 范围内
    void heal(float amount);
    // 消耗能量，返回是否消耗成功（能量不足则失败）
    bool consumeEnergy(float amount);
    // 恢复能量
    void restoreEnergy(float amount);
    // 判定本次攻击是否暴击，并返回最终计算出的子弹/飞弹伤害
    float calculateOutputDamage(float baseDamage, bool& outIsCrit) const;

    // ================= Getters & Setters =================
    // 生存属性
    float hp() const { return m_hp; }
    void setHp(float hp);

    float maxHp() const { return m_maxHp; }
    void setMaxHp(float maxHp);

    float energy() const { return m_energy; }
    void setEnergy(float energy);

    float maxEnergy() const { return m_maxEnergy; }
    void setMaxEnergy(float maxEnergy);

    float armor() const { return m_armor; }
    void setArmor(float armor) { m_armor = qMax(0.0f, armor); }

    // 攻击属性
    float machineGunDamage() const { return m_machineGunDamage; }
    void setMachineGunDamage(float damage) { m_machineGunDamage = qMax(0.0f, damage); }

    float missileDamage() const { return m_missileDamage; }
    void setMissileDamage(float damage) { m_missileDamage = qMax(0.0f, damage); }

    float critRate() const { return m_critRate; }
    void setCritRate(float rate) { m_critRate = qBound(0.0f, rate, 1.0f); }

    float critDamage() const { return m_critDamage; }
    void setCritDamage(float multiplier) { m_critDamage = qMax(1.0f, multiplier); }

    // 机动与状态属性
    float moveSpeed() const { return m_moveSpeed; }
    void setMoveSpeed(float speed) { m_moveSpeed = speed; }

    int invincibleDurationMs() const { return m_invincibleDurationMs; }
    void setInvincibleDurationMs(int ms) { m_invincibleDurationMs = qMax(0, ms); }

signals:
    // 数值发生变化时发出信号，UI 层（血条、能量条等）可绑定这些信号
    void hpChanged(float currentHp, float maxHp);
    void maxHpChanged(float maxHp);
    void energyChanged(float currentEnergy, float maxEnergy);
    void maxEnergyChanged(float maxEnergy);
    void died(); // 战机坠毁信号

private:
    // 生存属性
    float m_maxHp;
    float m_hp;
    float m_maxEnergy;  // 能量可以保留 int 或按需改 float
    float m_energy;
    float m_armor;    // 护甲值

    // 战斗属性
    float m_machineGunDamage;     // 机炮基础伤害
    float m_missileDamage;        // 飞弹基础伤害
    float m_critRate;           // 暴击率 (0.0 ~ 1.0)
    float m_critDamage;         // 暴击伤害倍率 (默认 1.5)

    // 机动与状态
    float m_moveSpeed;          // 左右平移速度 (像素/秒)
    int m_invincibleDurationMs; // 护盾无敌持续时间 (毫秒)
};
