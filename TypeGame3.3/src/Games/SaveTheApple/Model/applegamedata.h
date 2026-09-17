/* ------------------------------------------------------------------
// 文件名     : applegamedata.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果数据模型，维护关卡、分数、苹果列表等游戏核心状态
------------------------------------------------------------------ */
#pragma once
#include "gamedatabase.h"
#include "applegameconfig.h"
#include "appleentity.h"

// 拯救苹果游戏数据模型（MVC-Model）
class AppleGameData : public GameDataBase
{
    Q_OBJECT
public:
    explicit AppleGameData(AppleGameConfig* config, QObject* parent = nullptr);
    ~AppleGameData() override = default;

    // ========= 为了不破坏原有外部调用，提供薄封装 (Thin Wrappers) =========

    void addApple(AppleEntity* apple) {
        addEntity(apple);
    }

    void removeApple(AppleEntity* apple) {
        removeEntity(apple);
    }

    void removeAppleWithoutDestroy(AppleEntity* apple) {
        removeEntityWithoutDestroy(apple);
    }

    // 将基类的通用实体列表安全转换为特定实体列表，供View层渲染
    QList<AppleEntity*> getAliveApples() const {
        QList<AppleEntity*> apples;
        for (auto entity : m_aliveEntities) {
            apples.append(static_cast<AppleEntity*>(entity));
        }
        return apples;
    }
    AppleEntity* findAppleByLetter(QChar letter) const {
        return static_cast<AppleEntity*>(findEntityByLetter(letter));
    }
    // 配置快捷访问
    AppleGameConfig* getConfig() const {
        return static_cast<AppleGameConfig*>(m_config);
    }

};