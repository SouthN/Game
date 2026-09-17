/* ------------------------------------------------------------------
// 文件名     : gamedatabase.h
// 创建者     : [你的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : MVC 模型层基类，高度封装实体管理与计分核心机制
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QSet>
#include <QList>
#include <QTime>
#include "gameconfigbase.h"
#include "gameentity.h" // 引入实体基类

enum DataChangeType
{
    DATA_STATS_CHANGED = 0,    
    DATA_LEVEL_CHANGED,         
    DATA_ENTITY_CHANGED,        
    DATA_CONFIG_CHANGED         
};

class GameDataBase : public QObject
{
    Q_OBJECT

public:
    enum GameState
    {
        Idle = 0,
        Playing,
        Paused,
        LevelComplete, 
        GameOver
    };

    explicit GameDataBase(GameConfigBase* config, QObject* parent = nullptr);
    virtual ~GameDataBase();

    // ========= 禁用拷贝和赋值 =========
    GameDataBase(const GameDataBase&) = delete;
    GameDataBase& operator=(const GameDataBase&) = delete;

    // 通用的数据重置接口
    virtual void resetData();

    // ========== 核心实体管理 (共性上浮) ==========
    void addEntity(GameEntity* entity);
    void removeEntity(GameEntity* entity);
    void removeEntityWithoutDestroy(GameEntity* entity); // 仅剥离数据不销毁实例，供死亡动画使用
    
    QList<GameEntity*> getAliveEntities() const { return m_aliveEntities; }
    GameEntity* findEntityByLetter(QChar letter) const;

	QSet<QChar> getExistLetters() const { return m_existLetters; } // 当前场上存在的字母集合，供界面显示和重复输入判断使用
    void addExistLetter(QChar letter) { m_existLetters.insert(letter); }
    void removeExistLetter(QChar letter) { m_existLetters.remove(letter); }
    void clearExistLetters() { m_existLetters.clear(); }

    // ========== 状态与统计接口 ==========
    GameState getGameState() const { return m_gameState; }
    void setGameState(GameState state) {
        if (m_gameState != state) {
            m_gameState = state;
            emit stateChanged(m_gameState);
        }
    }

    GameConfigBase* getConfig() const { return m_config; }

    int getTotalSuccessCount() const { return m_totalSuccessCount; }
    int getTotalFailCount() const { return m_totalFailCount; }
    qint64 getTotalGameTimeMs() const;
    QString getFormattedTotalGameTime() const;

    void setSceneSize(qreal width, qreal height) {
        m_sceneWidth = width;
        m_sceneHeight = height;
        m_failLineY = height * 0.7;
    }
    qreal getSceneWidth() const { return m_sceneWidth; }
    qreal getSceneHeight() const { return m_sceneHeight; }
    qreal getFailLineY() const { return m_failLineY; }
    void setFailLineY(qreal y) { m_failLineY = y; }

signals:
    void dataChanged(int dataType);
    void stateChanged(int state);

protected:
	void initBaseConnection(); // 连接基础的信号槽，例如统计数据变化时通知界面更新

    GameState m_gameState;
    GameConfigBase* m_config = nullptr;

    qreal m_sceneWidth;
    qreal m_sceneHeight;
    qreal m_failLineY;

    QSet<QChar> m_existLetters;
    QList<GameEntity*> m_aliveEntities;

    int m_lastSuccessCount = 0;
    int m_totalSuccessCount = 0;
    int m_totalFailCount = 0;
    QTime m_gameStartTime;
};
