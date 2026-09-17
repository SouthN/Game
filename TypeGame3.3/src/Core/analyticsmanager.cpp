#include "analyticsmanager.h"
#include <QUuid>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QJsonArray>

// ==================== AnalyticsWorker 实现 ====================
void AnalyticsWorker::initAnalytics(const QString& dataDir)
{
    QMutexLocker locker(&m_cacheMutex);
    m_dataDirectory = dataDir;
    m_sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 【修改】确保数据目录存在，并检查创建结果
    QDir dir(dataDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCritical() << tr("[Analytics] Failed to create data directory: %1").arg(dataDir);
            return;
        }
    }
    // 【新增】输出数据目录绝对路径，方便用户查找
    qInfo() << tr("[Analytics] Data directory: %1").arg(QDir::toNativeSeparators(m_dataDirectory));

    // 加载缓存的事件
    loadEventsFromCache();

    // 启动定时上报定时器
    m_flushTimer = new QTimer(this);
    connect(m_flushTimer, &QTimer::timeout, this, &AnalyticsWorker::onFlushTimer);
    m_flushTimer->start(m_flushInterval);
    qInfo() << tr("[Analytics] Initialized successfully, session ID: %1").arg(m_sessionId);
}

// 【修改】工作线程接收整个批次的事件
void AnalyticsWorker::trackEventBatch(const AnalyticsEventList& events)
{
    QMutexLocker locker(&m_cacheMutex);
    m_eventCache.append(events);

    // 如果总缓存达到上限，立即落盘/上报
    if (m_eventCache.size() >= m_maxCacheSize) {
        flushEvents();
    }
}


void AnalyticsWorker::flushEvents()
{
    QMutexLocker locker(&m_cacheMutex);
    if (m_eventCache.isEmpty()) {
        qDebug() << tr("[Analytics] No events to flush");
        return;
    }

    // 复制事件列表并清空缓存
    QList<AnalyticsEvent> eventsToSend = m_eventCache;
    m_eventCache.clear();

    // 解锁后再执行网络操作，避免长时间持有锁
    locker.unlock();

    // 【新增】日志输出
    qInfo() << tr("[Analytics] Flushing %1 events to server").arg(eventsToSend.size());

    // 发送到服务器（这里模拟上报）
    sendEventsToServer(eventsToSend);
}

void AnalyticsWorker::releaseAll()
{
    QMutexLocker locker(&m_cacheMutex);
    if (m_flushTimer) {
        m_flushTimer->stop();
    }
    // 最后一次上报
    if (!m_eventCache.isEmpty()) {
        saveEventsToCache();
    }
    qInfo() << tr("[Analytics] Resources released, cached events: %1").arg(m_eventCache.size());
}

void AnalyticsWorker::onFlushTimer()
{
    flushEvents();
}

void AnalyticsWorker::saveEventsToCache()
{
    if (m_eventCache.isEmpty()) {
        qDebug() << tr("[Analytics] No events to save to cache");
        return;
    }

    QJsonArray eventArray;
    for (const AnalyticsEvent& event : m_eventCache) {
        QJsonObject eventObj;
        eventObj["eventId"] = event.eventId;
        eventObj["eventName"] = event.eventName;
        eventObj["properties"] = event.properties;
        eventObj["timestamp"] = event.timestamp;
        eventObj["sessionId"] = event.sessionId;
        eventArray.append(eventObj);
    }

    QJsonDocument doc(eventArray);
    QString cacheFilePath = m_dataDirectory + "/analytics_cache.json";
    QFile cacheFile(cacheFilePath);

    // 【修改】增加文件打开模式和错误检查
    if (!cacheFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCritical() << tr("[Analytics] Failed to open cache file: %1, error: %2")
            .arg(QDir::toNativeSeparators(cacheFilePath))
            .arg(cacheFile.errorString());
        return;
    }

    // 【修改】增加写入结果检查
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    qint64 bytesWritten = cacheFile.write(jsonData);
    if (bytesWritten == -1) {
        qCritical() << tr("[Analytics] Failed to write cache file: %1, error: %2")
            .arg(QDir::toNativeSeparators(cacheFilePath))
            .arg(cacheFile.errorString());
    }
    else {
        qInfo() << tr("[Analytics] Saved %1 events to cache: %2")
            .arg(m_eventCache.size())
            .arg(QDir::toNativeSeparators(cacheFilePath));
    }
}

void AnalyticsWorker::loadEventsFromCache()
{
    QString cacheFilePath = m_dataDirectory + "/analytics_cache.json";
    QFile cacheFile(cacheFilePath);

    if (!cacheFile.exists()) {
        qDebug() << tr("[Analytics] No cache file found at: %1").arg(QDir::toNativeSeparators(cacheFilePath));
        return;
    }
    // 【修改】增加文件打开错误检查
    if (!cacheFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << tr("[Analytics] Failed to open cache file: %1, error: %2")
            .arg(QDir::toNativeSeparators(cacheFilePath))
            .arg(cacheFile.errorString());
        return;
    }

    QByteArray data = cacheFile.readAll();
    cacheFile.close();

    // 【修改】增加JSON解析错误检查
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << tr("[Analytics] Failed to parse cache file: %1, error: %2")
            .arg(QDir::toNativeSeparators(cacheFilePath))
            .arg(parseError.errorString());
        return;
    }

    if (doc.isArray()) {
        QJsonArray eventArray = doc.array();
        for (const QJsonValue& value : eventArray) {
            if (value.isObject()) {
                QJsonObject eventObj = value.toObject();
                AnalyticsEvent event;
                event.eventId = eventObj["eventId"].toString();
                event.eventName = eventObj["eventName"].toString();
                event.properties = eventObj["properties"].toObject();
                event.timestamp = eventObj["timestamp"].toVariant().toLongLong();
                event.sessionId = eventObj["sessionId"].toString();
                m_eventCache.append(event);
            }
        }
        qInfo() << tr("[Analytics] Loaded %1 events from cache").arg(m_eventCache.size());
    }

    // 删除缓存文件
    if (!QFile::remove(cacheFilePath)) {
        qWarning() << tr("[Analytics] Failed to delete cache file: %1").arg(QDir::toNativeSeparators(cacheFilePath));
    }

}

void AnalyticsWorker::sendEventsToServer(const QList<AnalyticsEvent>& events)
{
    // 这里应该实现实际的网络上报逻辑
    // 为了演示，我们只打印日志
    qInfo() << tr("[Analytics] Reporting events, count: %1").arg(events.size());
    for (const AnalyticsEvent& event : events) {
        qDebug() << tr("  - Event: %1, Time: %2")
            .arg(event.eventName)
            .arg(QDateTime::fromMSecsSinceEpoch(event.timestamp).toString("hh:mm:ss.zzz"));
    }
}

// ==================== AnalyticsManager 实现 ====================
AnalyticsManager::AnalyticsManager(QObject* parent) : QObject(parent)
{
    qRegisterMetaType<AnalyticsEvent>(); //注册元类型
    // 【新增】注册批处理元类型
    qRegisterMetaType<AnalyticsEventList>("AnalyticsEventList");

    m_sessionId = generateSessionId();
    initWorkerThread();
}

AnalyticsManager::~AnalyticsManager()
{
    sig_releaseAll();
    if (m_analyticsThread) {
        m_analyticsThread->quit();
        m_analyticsThread->wait(3000);
    }
}

void AnalyticsManager::init(const QString& dataDir)
{
    QString actualDataDir = dataDir;
    if (actualDataDir.isEmpty()) {
        actualDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/analytics";
    }
    emit sig_initAnalytics(actualDataDir);
}

void AnalyticsManager::setCommonProperty(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_propertyMutex);
    m_commonProperties[key] = QJsonValue::fromVariant(value);
}

void AnalyticsManager::removeCommonProperty(const QString& key)
{
    QMutexLocker locker(&m_propertyMutex);
    m_commonProperties.remove(key);
}

void AnalyticsManager::trackEvent(const QString& eventName, const QJsonObject& properties)
{
    AnalyticsEvent event;
    event.eventId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    event.eventName = eventName;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.sessionId = m_sessionId;

    // 合并公共属性
    {
        QMutexLocker locker(&m_propertyMutex);
        event.properties = m_commonProperties;
    }

    // 合并事件特定属性
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        event.properties[it.key()] = it.value();
    }

    // ========== 【核心优化：积攒到 UI 线程缓存，而非立即发送信号】 ==========
    {
        QMutexLocker locker(&m_batchMutex);
        m_eventBatch.append(event);
    }
}

void AnalyticsManager::trackPageView(const QString& pageName, const QString& pageId)
{
    QJsonObject props;
    props["pageName"] = pageName;
    if (!pageId.isEmpty()) {
        props["pageId"] = pageId;
    }
    trackEvent("page_view", props);
}

void AnalyticsManager::trackButtonClick(const QString& buttonName, const QString& buttonId)
{
    QJsonObject props;
    props["buttonName"] = buttonName;
    if (!buttonId.isEmpty()) {
        props["buttonId"] = buttonId;
    }
    trackEvent("button_click", props);
}

void AnalyticsManager::trackLevelEvent(int level, const QString& eventType, int score)
{
    QJsonObject props;
    props["level"] = level;
    props["eventType"] = eventType;
    if (score > 0) {
        props["score"] = score;
    }
    trackEvent("level_event", props);
}

// 【新增】定时向 Worker 线程发射整批数据
void AnalyticsManager::onBatchTimer()
{
    QMutexLocker locker(&m_batchMutex);
    if (m_eventBatch.isEmpty()) return;

    // 一次性跨线程投递 N 个事件，只引发 1 次深拷贝和事件队列锁
    emit sig_trackEventBatch(m_eventBatch);
    m_eventBatch.clear();
}

void AnalyticsManager::flush()
{
    // 强制先将 UI 线程的缓存推入 Worker
    onBatchTimer();
    emit sig_flushEvents();
}

void AnalyticsManager::initWorkerThread()
{
    m_analyticsThread = new QThread(this);
    m_analyticsWorker = new AnalyticsWorker();
    m_analyticsWorker->moveToThread(m_analyticsThread);

    // 连接信号槽
    connect(this, &AnalyticsManager::sig_initAnalytics, m_analyticsWorker, &AnalyticsWorker::initAnalytics);
    connect(this, &AnalyticsManager::sig_trackEventBatch, m_analyticsWorker, &AnalyticsWorker::trackEventBatch);
    connect(this, &AnalyticsManager::sig_flushEvents, m_analyticsWorker, &AnalyticsWorker::flushEvents);
    connect(this, &AnalyticsManager::sig_releaseAll, m_analyticsWorker, &AnalyticsWorker::releaseAll);

    connect(m_analyticsThread, &QThread::finished, m_analyticsWorker, &QObject::deleteLater);
    m_analyticsThread->start();

    // ========== 【新增】启动 UI 线程批处理定时器 ==========
    m_batchTimer = new QTimer(this);
    connect(m_batchTimer, &QTimer::timeout, this, &AnalyticsManager::onBatchTimer);
    m_batchTimer->start(500); // 每 500ms 打包投递一次
}

QString AnalyticsManager::generateSessionId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

