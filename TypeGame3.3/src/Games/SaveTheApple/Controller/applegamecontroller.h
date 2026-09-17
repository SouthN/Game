/* ------------------------------------------------------------------
// 文件名     : applegamecontroller.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果逻辑控制器，处理苹果生成、下落、键盘匹配及胜负判定
------------------------------------------------------------------ */
#pragma once
#include "gamecontrollerbase.h"
#include "applegamedata.h"
#include "applegameview.h"


// 拯救苹果游戏控制器（MVC-Controller）
class AppleGameController : public GameControllerBase
{
    Q_OBJECT
public:
    explicit AppleGameController(AppleGameData* data, AppleGameView* view, QObject* parent = nullptr);
	void initConnection(AppleGameData* data, AppleGameView* view); // 连接数据和视图的信号槽关系，单独封装为方法，便于在重置游戏时重新连接
    ~AppleGameController() override = default;

protected:
    // =========================================================
    // 实现基类的模板方法钩子
    // =========================================================
    GameEntity* createNewEntity() override;
    void updateViewEntities() override;
    void removeEntityFromView(GameEntity* entity) override;
    void clearSuccessIconsFromView() override;
    void playHitSound() override;
    void playMissSound() override;
    int getEntityWidth() const override;
    int getEntityHeight() const override;

private:
    AppleGameData* getGameData() { return static_cast<AppleGameData*>(m_gameData); }
    AppleGameView* getGameView() { return static_cast<AppleGameView*>(m_gameView); }
};