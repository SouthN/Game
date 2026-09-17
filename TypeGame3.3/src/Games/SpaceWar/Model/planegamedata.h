/* ------------------------------------------------------------------
// 文件名     : planegamedata.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 太空大战数据模型，维护关卡、分数、敌机列表等游戏核心状态
------------------------------------------------------------------ */
#pragma once
#include "gamedatabase.h"
#include "planegameconfig.h"
#include "planeentity.h"
#include "bossentity.h"

class PlaneGameData : public GameDataBase
{
    Q_OBJECT
public:
    explicit PlaneGameData(PlaneGameConfig* config, QObject* parent = nullptr);
    ~PlaneGameData() override = default;

    // ========= 为了不破坏原有外部调用，提供薄封装 (Thin Wrappers) =========

    void addPlane(PlaneEntity* plane) {
        addEntity(plane);
    }

    void removePlane(PlaneEntity* plane) {
        removeEntity(plane);
    }

    void removePlaneWithoutDestroy(PlaneEntity* plane) {
        removeEntityWithoutDestroy(plane);
    }

    // 将基类的通用实体列表安全转换为特定实体列表，供View层渲染
    QList<PlaneEntity*> getAlivePlanes() const {
        QList<PlaneEntity*> planes;
        for (auto entity : m_aliveEntities) {
            // 【核心修复】：使用 qobject_cast 安全过滤，剔除掉 RewardWordEntity
            PlaneEntity* plane = qobject_cast<PlaneEntity*>(entity);
            if (plane) {
                planes.append(plane);
            }
        }
        return planes;
    }

    PlaneEntity* findPlaneByLetter(QChar letter) const {
        GameEntity* entity = findEntityByLetter(letter);
        // 【核心修复】
        return qobject_cast<PlaneEntity*>(entity);
    }

    PlaneGameConfig* getConfig() const {
        return static_cast<PlaneGameConfig*>(m_config);
    }

	// ========================= Boss战相关接口 =========================
	BossEntity* getActiveBoss() const { return m_activeBoss; } // 获取当前活跃的Boss实体指针
	void setActiveBoss(BossEntity* boss) { m_activeBoss = boss; } // 设置当前活跃的Boss实体指针，供Boss战逻辑使用

private:
	BossEntity* m_activeBoss = nullptr; // 当前活跃的Boss实体指针，若无Boss则为nullptr
};