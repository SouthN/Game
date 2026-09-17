/* ------------------------------------------------------------------
// 文件名     : analyticsmanager.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 埋点与数据统计管理器，负责追踪玩家行为与游戏核心事件
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QTimer>
#include "singleton.h"

// 双重定时器
// 第一级缓冲：m_batchTimer(UI 线程->Worker 线程) 解决的痛点：跨线程通信（Signal / Slot）的开销和锁竞争
// 第二级缓冲：m_flushTimer(Worker 线程->磁盘 / 服务器) 解决的痛点：磁盘 I / O 或 网络 HTTP 请求的开销。

// 埋点事件结构体
struct AnalyticsEvent
{
    QString eventId;          // 事件ID
    QString eventName;        // 事件名称
    QJsonObject properties;   // 事件属性
    qint64 timestamp;         // 时间戳
    QString sessionId;        // 会话ID
};
Q_DECLARE_METATYPE(AnalyticsEvent)

// ========== 【核心优化：注册批量列表类型】 ==========
typedef QList<AnalyticsEvent> AnalyticsEventList;
Q_DECLARE_METATYPE(AnalyticsEventList)
// =================================================

// ==================== AnalyticsWorker 工作类 ====================
class AnalyticsWorker : public QObject
{
    Q_OBJECT
public slots:
    // 初始化埋点系统
    void initAnalytics(const QString& dataDir);
    // 【修改】接收批处理事件，替代原来的单个接收
    void trackEventBatch(const AnalyticsEventList& events);
    // 强制上报所有缓存事件
    void flushEvents();
    // 释放资源
    void releaseAll();

private slots:
    // 定时上报
    void onFlushTimer();

private:
    void saveEventsToCache();
    void loadEventsFromCache();
    void sendEventsToServer(const QList<AnalyticsEvent>& events);

    QList<AnalyticsEvent> m_eventCache;
    QMutex m_cacheMutex;
    QString m_dataDirectory;
    QString m_sessionId;
    QTimer* m_flushTimer = nullptr;
    //int m_flushInterval = 30000; // 30秒上报一次
    //int m_maxCacheSize = 100;     // 缓存上限
    int m_flushInterval = 30000; // 30秒上报一次
    int m_maxCacheSize = 10;     // 缓存上限
};

// ==================== AnalyticsManager 管理器 ====================
class AnalyticsManager : public QObject, public Singleton<AnalyticsManager>
{
    Q_OBJECT
    friend class Singleton<AnalyticsManager>;
public:
    ~AnalyticsManager() override;

    // ========= 新增：显式禁用拷贝和赋值 =========
    AnalyticsManager(const AnalyticsManager&) = delete;
    AnalyticsManager& operator=(const AnalyticsManager&) = delete;
    // ===========================================

    // 初始化埋点系统
    void init(const QString& dataDir = QString());
    // 设置公共属性（会添加到所有事件中）
    void setCommonProperty(const QString& key, const QVariant& value);
    // 移除公共属性
    void removeCommonProperty(const QString& key);

    // ========== 埋点记录接口（主线程调用） ==========
    // 自定义事件
    void trackEvent(const QString& eventName, const QJsonObject& properties = QJsonObject());
    // 页面浏览事件
    void trackPageView(const QString& pageName, const QString& pageId = QString());
    // 按钮点击事件
    void trackButtonClick(const QString& buttonName, const QString& buttonId = QString());
    // 游戏关卡事件
    void trackLevelEvent(int level, const QString& eventType, int score = 0);
    // 强制上报
    void flush();

signals:
    // 信号：转发到工作线程
    void sig_initAnalytics(const QString& dataDir);
    // 【修改】发送批处理信号
    void sig_trackEventBatch(const AnalyticsEventList& events);
    void sig_flushEvents();
    void sig_releaseAll();

private slots:
    // 【新增】UI 线程的批处理定时器槽函数
    void onBatchTimer();

private:
    explicit AnalyticsManager(QObject* parent = nullptr);
    void initWorkerThread();
    QString generateSessionId() const;

    QThread* m_analyticsThread = nullptr;
    AnalyticsWorker* m_analyticsWorker = nullptr;
    QJsonObject m_commonProperties;
    QString m_sessionId;
    QMutex m_propertyMutex;

    // ========== 【核心优化：批处理缓存】 ==========
    AnalyticsEventList m_eventBatch;
    QMutex m_batchMutex;
    QTimer* m_batchTimer = nullptr;
};

// ========== 便捷宏定义 ==========
#define ANALYTICS_PAGE(page) AnalyticsManager::instance()->trackPageView(page)
#define ANALYTICS_BUTTON(button) AnalyticsManager::instance()->trackButtonClick(button)
#define ANALYTICS_EVENT(name, props) AnalyticsManager::instance()->trackEvent(name, props)