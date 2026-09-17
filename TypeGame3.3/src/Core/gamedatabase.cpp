#include "gamedatabase.h"

GameDataBase::GameDataBase(GameConfigBase* config, QObject* parent)
    : QObject(parent), m_gameState(Idle), m_config(config),
      m_sceneWidth(1250), m_sceneHeight(850), m_failLineY(595.0)
{
    initBaseConnection();
}

GameDataBase::~GameDataBase()
{
    // 利用多态的安全销毁机制
    for (auto entity : m_aliveEntities) {
        entity->destroy();
    }
    m_aliveEntities.clear();
    m_existLetters.clear();
}

void GameDataBase::initBaseConnection()
{
    if (m_config) {
        connect(m_config, &GameConfigBase::statsChanged, this, [this]() {
            int success = m_config->getSuccessCount();
            if (success > m_lastSuccessCount) {
                emit dataChanged(DATA_STATS_CHANGED);
            }
            m_lastSuccessCount = success;
        });
        connect(m_config, &GameConfigBase::successAdded, this, [this]() {
            m_totalSuccessCount++;
        });
        connect(m_config, &GameConfigBase::failAdded, this, [this]() {
            m_totalFailCount++;
        });
    }
}

void GameDataBase::resetData()
{
    for (auto entity : m_aliveEntities) {
        entity->destroy();
    }
    m_aliveEntities.clear();
    m_existLetters.clear();

    if (m_config) m_config->resetStats();
    m_lastSuccessCount = 0;
    m_totalSuccessCount = 0;
    m_totalFailCount = 0;
    
    // 重置全局开始时间
    m_gameStartTime = QTime::currentTime();

    emit dataChanged(DATA_ENTITY_CHANGED);
    emit dataChanged(DATA_STATS_CHANGED);
}

void GameDataBase::addEntity(GameEntity* entity)
{
    if (!entity || m_aliveEntities.contains(entity)) return;
    m_aliveEntities.append(entity);
    m_existLetters.insert(entity->getLetter());
    emit dataChanged(DATA_ENTITY_CHANGED);
}

void GameDataBase::removeEntity(GameEntity* entity)
{
    if (!entity || !m_aliveEntities.contains(entity)) return;
    m_existLetters.remove(entity->getLetter());
    m_aliveEntities.removeOne(entity);
    
    // 依赖多态，让实体自己找到对应的工厂内存池进行销毁！
    entity->destroy(); 
    emit dataChanged(DATA_ENTITY_CHANGED);
}

void GameDataBase::removeEntityWithoutDestroy(GameEntity* entity)
{
    if (!entity || !m_aliveEntities.contains(entity)) return; 

    m_aliveEntities.removeOne(entity);
    emit dataChanged(DATA_ENTITY_CHANGED);
}

GameEntity* GameDataBase::findEntityByLetter(QChar letter) const
{
    for (auto entity : m_aliveEntities) {
        // 防止被重复击中
        if (entity->getLetter() == letter && entity->isAlive() && !entity->isSelected()) {
            return entity; 
        }
    }
    return nullptr;
}

qint64 GameDataBase::getTotalGameTimeMs() const
{
    if (!m_gameStartTime.isValid()) return 0;
    return m_gameStartTime.msecsTo(QTime::currentTime());
}

QString GameDataBase::getFormattedTotalGameTime() const
{
    qint64 elapsedMs = getTotalGameTimeMs();
    int seconds = (elapsedMs / 1000) % 60;
    int minutes = (elapsedMs / 1000) / 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}