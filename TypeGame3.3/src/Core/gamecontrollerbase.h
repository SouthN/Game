/* ------------------------------------------------------------------
// 文件名     : gamecontrollerbase.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : MVC 控制器基类，负责业务逻辑、游戏循环控制及事件输入处理
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QTimer>
#include <QKeyEvent>
#include <QElapsedTimer>
#include "gamedatabase.h"
#include "gameviewbase.h"

// MVC Controller 基类：负责业务逻辑、游戏循环、输入处理，连接Model与View
class GameControllerBase : public QObject
{
    Q_OBJECT
public:
    explicit GameControllerBase(GameDataBase* data, GameViewBase* view, QObject* parent = nullptr);
    virtual ~GameControllerBase();

    // ========= 新增：禁用拷贝和赋值 =========
    GameControllerBase(const GameControllerBase&) = delete;
    GameControllerBase& operator=(const GameControllerBase&) = delete;
    // ========================================

    // 测试模式接口
    void setTestMode(bool isTest) { m_isTestMode = isTest; }
    void manualTick() { if (m_isTestMode) onGameLoop(); }

    // 游戏生命周期控制接口
    virtual void startGame();
    virtual void pauseGame();
    virtual void resumeGame();
    virtual void stopGame();

public slots:
    // 数据与状态响应（默认空实现，子类可覆盖）
    virtual void onDataChanged(int dataType) {}
    virtual void onStateChanged(int state);

    // 核心输入处理
    virtual void handleKeyPress(QKeyEvent* event);

    // 通用的弹窗按钮槽函数
    virtual void onNextLevelClicked(); // 关卡完成后进入下一关
    virtual void onRestartGameClicked(); // 失败或完成后重试当前关卡
    virtual void onExitGameClicked();  // 失败或完成后退出游戏
    virtual void onLevelChanged(int level);  // 关卡变化时更新实体速度等参数

signals:
    void exitGame(); // 统一向上层发送的退出信号

protected slots:
    // 游戏主循环
    virtual void onGameLoop();

protected:
    // =========================================================
    // 【核心设计：模板方法钩子】
    // 将两个游戏唯一有差异的行为，作为纯虚函数交由子类实现
    // =========================================================
    virtual GameEntity* createNewEntity() = 0;              // 创建具体实体
    virtual void updateViewEntities() = 0;                  // 更新视图中实体的渲染
    virtual void removeEntityFromView(GameEntity* entity) = 0;// 从视图移除实体
    virtual void clearSuccessIconsFromView() = 0;           // 清除视图中的成功图标
    virtual void playHitSound() = 0;                        // 播放击中音效
    virtual void playMissSound() = 0;                       // 播放丢失音效
    virtual int getEntityWidth() const = 0;                 // 获取实体逻辑宽度
    virtual int getEntityHeight() const = 0;                // 获取实体逻辑高度
    virtual int getSpawnEntityCount() const;                // 获取用于控制补充生成的实体数量
    // 【新增】提供给子类更新专属物理逻辑（如玩家战机）的钩子
    virtual void updateCustomPhysics(float deltaTime) {}

    // =========================================================
    // 提取出的公共业务逻辑
    // =========================================================
    void spawnEntity(); // 根据当前关卡配置生成新实体
    void checkEntityOutOfBounds(); // 检查实体是否越界（触底），并处理失败逻辑
    virtual void checkPassCondition();  // 检查过关条件，触发过关或游戏结束逻辑
    void clearAllEntities();  // 清除所有实体，供重试或下一关使用


    GameDataBase* m_gameData = nullptr;
    GameViewBase* m_gameView = nullptr;
    QTimer* m_gameLoopTimer = nullptr;
    static constexpr int GAME_LOOP_INTERVAL = 16; // 60帧

    // ========= 新增：测试模式标志 =========
    bool m_isTestMode = false;
    QElapsedTimer m_frameTimer; // 用于计算真实的帧间隔时间

    // =========================================================
    // 【新增】固定物理步长相关变量
    // =========================================================
    float m_accumulator = 0.0f; // 物理时间累加器
    static constexpr float FIXED_TIME_STEP = 0.016f; // 固定物理步长 16ms

private:
    void initGameLoop(); // 初始化游戏循环定时器
};
