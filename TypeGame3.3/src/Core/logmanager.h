/* ------------------------------------------------------------------
// 文件名     : logmanager.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 日志管理系统，支持多级别日志记录及格式化输出
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "singleton.h"

// 日志级别枚举
enum LogLevel
{
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
};
Q_DECLARE_METATYPE(LogLevel)

// ==================== LogWorker 工作类 ====================
class LogWorker : public QObject
{
    Q_OBJECT
public slots:
    // 初始化日志文件
    void initLogFile(const QString& logDir);
    // 写入日志（线程内执行）
    void writeLog(LogLevel level, const QString& message, const QString& file, int line, const QString& function);
    // 清理过期日志
    void cleanOldLogs(int keepDays);
    // 释放资源
    void releaseAll();

private:
    QString getLogLevelString(LogLevel level) const;
    QString getCurrentLogFileName() const;

    QFile* m_logFile = nullptr;
    QTextStream* m_logStream = nullptr;
    QMutex m_fileMutex;
    QString m_logDirectory;
};

// ==================== LogManager 管理器 ====================
class LogManager : public QObject, public Singleton<LogManager>
{
    Q_OBJECT
    friend class Singleton<LogManager>;
public:
    ~LogManager() override;

    // ========= 新增：显式禁用拷贝和赋值 =========
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    // ===========================================

    // 初始化日志系统
    void init(const QString& logDir = QString());
    // 设置日志级别（低于此级别的日志不会被记录）
    void setLogLevel(LogLevel level);
    // 清理过期日志
    void cleanOldLogs(int keepDays = 7);

    // ========== 日志记录接口（主线程调用） ==========
    void debug(const QString& message, const char* file = nullptr, int line = 0, const char* function = nullptr);
    void info(const QString& message, const char* file = nullptr, int line = 0, const char* function = nullptr);
    void warning(const QString& message, const char* file = nullptr, int line = 0, const char* function = nullptr);
    void error(const QString& message, const char* file = nullptr, int line = 0, const char* function = nullptr);
    void critical(const QString& message, const char* file = nullptr, int line = 0, const char* function = nullptr);

signals:
    // 信号：转发到工作线程
    void sig_initLogFile(const QString& logDir);
    void sig_writeLog(LogLevel level, const QString& message, const QString& file, int line, const QString& function);
    void sig_cleanOldLogs(int keepDays);
    void sig_releaseAll();

private:
    explicit LogManager(QObject* parent = nullptr);
    void initWorkerThread();

    QThread* m_logThread = nullptr;
    LogWorker* m_logWorker = nullptr;
    LogLevel m_minLogLevel = LOG_DEBUG;
};

// ========== 便捷宏定义 ==========
#define LOG_DEBUG(msg) LogManager::instance()->debug(msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_INFO(msg) LogManager::instance()->info(msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_WARNING(msg) LogManager::instance()->warning(msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_ERROR(msg) LogManager::instance()->error(msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_CRITICAL(msg) LogManager::instance()->critical(msg, __FILE__, __LINE__, Q_FUNC_INFO)