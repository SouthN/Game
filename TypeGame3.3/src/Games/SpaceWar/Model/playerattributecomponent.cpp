/* ------------------------------------------------------------------
// 文件名     : playerattributecomponent.cpp
// ------------------------------------------------------------------ */
#include "playerattributecomponent.h"
#include <QRandomGenerator>


PlayerAttributeComponent::PlayerAttributeComponent(QObject* parent)
    : QObject(parent)
{
    initDefaultStats();
}

void PlayerAttributeComponent::initDefaultStats()
{
    // 这里的值是为了兼容当前 PlayerEntity 的设定 (原 m_maxHp = 18)
    m_maxHp = 18.0f;
    m_hp = 18.0f;

    m_maxEnergy = 100.0f;
    m_energy = 100.0f;

    m_armor = 0.0f;              // 初始无护甲减伤

    m_machineGunDamage = 1.0f;   // 默认机炮伤害
    m_missileDamage = 5.0f;     // 默认飞弹伤害

    m_critRate = 0.05f;       // 初始 5% 暴击率
    m_critDamage = 1.5f;      // 暴击造成 1.5 倍伤害

    m_moveSpeed = 500.0f;     // 默认移动速度 (像素/秒)
    m_invincibleDurationMs = 1000; // 默认无敌时间为 1000 毫秒
}

float PlayerAttributeComponent::applyDamage(float rawDamage)
{
    if (m_hp <= 0.0f) return 0.0f; // 已经坠毁

    // 计算护甲减伤，保底造成 1 点伤害（破甲机制：哪怕护甲再高，也要强制扣1滴血，除非伤害本身为0）
    float actualDamage = rawDamage - m_armor;
    if (actualDamage <= 0.0f && rawDamage > 0.0f) {
        actualDamage = 1.0f;
    }

    setHp(m_hp - actualDamage);
    return actualDamage;
}

void PlayerAttributeComponent::heal(float amount)
{
    if (m_hp <= 0.0f || amount <= 0.0f) return;
    setHp(m_hp + amount);
}

bool PlayerAttributeComponent::consumeEnergy(float amount)
{
    if (m_energy >= amount) {
        setEnergy(m_energy - amount);
        return true;
    }
    return false;
}

void PlayerAttributeComponent::restoreEnergy(float amount)
{
    if (amount <= 0.0f) return;
    setEnergy(m_energy + amount);
}

float PlayerAttributeComponent::calculateOutputDamage(float baseDamage, bool& outIsCrit) const
{
    outIsCrit = false;
    float finalDamage = baseDamage;

    float randomVal = static_cast<float>(QRandomGenerator::global()->generateDouble());
    if (randomVal <= m_critRate) {
        outIsCrit = true;
        finalDamage *= m_critDamage;
    }

    return finalDamage;
}

void PlayerAttributeComponent::setHp(float hp)
{
    float newHp = qBound(0.0f, hp, m_maxHp);
    // 使用 qAbs 解决浮点数精度比对问题
    if (qAbs(m_hp - newHp) > 0.001f) {
        m_hp = newHp;
        emit hpChanged(m_hp, m_maxHp);

        if (m_hp <= 0.0f) {
            emit died();
        }
    }
}

void PlayerAttributeComponent::setMaxHp(float maxHp)
{
    float newMaxHp = qMax(1.0f, maxHp);
	if (qAbs(m_maxHp - newMaxHp) > 0.001f) { // 使用 qAbs 解决浮点数精度比对问题
        m_maxHp = newMaxHp;
        emit maxHpChanged(m_maxHp);

        if (m_hp > m_maxHp) {
            setHp(m_maxHp);
        }
        else {
            emit hpChanged(m_hp, m_maxHp); // 虽然当前血量数值没变，但最大血量变了，也应通知UI更新比例
        }
    }
}

void PlayerAttributeComponent::setEnergy(float energy)
{
    float newEnergy = qBound(0.0f, energy, m_maxEnergy);
    if (qAbs(m_energy - newEnergy) > 0.001f) { // 使用 qAbs 解决浮点数精度比对问题
        m_energy = newEnergy;
        emit energyChanged(m_energy, m_maxEnergy);
    }
}

void PlayerAttributeComponent::setMaxEnergy(float maxEnergy)
{
    float newMaxEnergy = qMax(0.0f, maxEnergy);
    if (qAbs(m_maxEnergy - newMaxEnergy) > 0.001f) { // 使用 qAbs 解决浮点数精度比对问题
        m_maxEnergy = newMaxEnergy;
        emit maxEnergyChanged(m_maxEnergy);

        if (m_energy > m_maxEnergy) {
            setEnergy(m_maxEnergy);
        }
        else {
            emit energyChanged(m_energy, m_maxEnergy);
        }
    }
}

