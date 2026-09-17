#include "logmanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

// ==================== LogWorker 实现 ====================
void LogWorker::initLogFile(const QString& logDir)
{
    QMutexLocker locker(&m_fileMutex);
    m_logDirectory = logDir;

    // 确保日志目录存在
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 打开日志文件
    QString logFileName = getCurrentLogFileName();
    m_logFile = new QFile(logFileName);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logStream = new QTextStream(m_logFile);
        m_logStream->setCodec("UTF-8");
        qInfo() << tr("[Log System] Initialized successfully, log file: %1").arg(logFileName);
    } else {
        qWarning() << tr("[Log System] Failed to open log file: %1").arg(logFileName);
    }
}

void LogWorker::writeLog(LogLevel level, const QString& message, const QString& file, int line, const QString& function)
{
    QMutexLocker locker(&m_fileMutex);
    if (!m_logStream) return;

    // 格式化日志内容
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = getLogLevelString(level);
    
    QString logMessage = QString("[%1] [%2] %3")
        .arg(timestamp)
        .arg(levelStr)
        .arg(message);

    // 如果有文件信息，追加到日志
    if (!file.isEmpty()) {
        logMessage += QString(" (at %1:%2, %3)")
            .arg(file)
            .arg(line)
            .arg(function);
    }

    // 写入文件并刷新
    *m_logStream << logMessage << "\n";
    m_logStream->flush();

    // 同时输出到控制台
    switch (level) {
        case LOG_DEBUG: qDebug().noquote() << logMessage; break;
        case LOG_INFO: qInfo().noquote() << logMessage; break;
        case LOG_WARNING: qWarning().noquote() << logMessage; break;
        case LOG_ERROR: qCritical().noquote() << logMessage; break;
        case LOG_CRITICAL: qCritical().noquote() << logMessage; break;
    }
}

void LogWorker::cleanOldLogs(int keepDays)
{
    QMutexLocker locker(&m_fileMutex);
    QDir logDir(m_logDirectory);
    if (!logDir.exists()) return;

    QFileInfoList fileList = logDir.entryInfoList(QStringList() << "*.log", QDir::Files, QDir::Time);
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-keepDays);

    for (const QFileInfo& fileInfo : fileList) {
        if (fileInfo.lastModified() < cutoffDate) {
            QFile::remove(fileInfo.absoluteFilePath());
            qInfo() << tr("[Log System] Deleted expired log file: %1").arg(fileInfo.fileName());
        }
    }
}

void LogWorker::releaseAll()
{
    QMutexLocker locker(&m_fileMutex);
    if (m_logStream) {
        m_logStream->flush();
        delete m_logStream;
        m_logStream = nullptr;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
    qInfo() << tr("[Log System] Log system resources released");
}

QString LogWorker::getLogLevelString(LogLevel level) const
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        case LOG_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

QString LogWorker::getCurrentLogFileName() const
{
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    return m_logDirectory + QString("/game_%1.log").arg(dateStr);
}

// ==================== LogManager 实现 ====================
LogManager::LogManager(QObject* parent) : QObject(parent)
{
    qRegisterMetaType<LogLevel>(); //注册元类型
    initWorkerThread();
}

LogManager::~LogManager()
{
    sig_releaseAll();
    if (m_logThread) {
        m_logThread->quit();
        m_logThread->wait(3000);
    }
}

void LogManager::init(const QString& logDir)
{
    QString actualLogDir = logDir;
    if (actualLogDir.isEmpty()) {
        // 默认使用应用数据目录
        actualLogDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    }
    emit sig_initLogFile(actualLogDir);
}

void LogManager::setLogLevel(LogLevel level)
{
    m_minLogLevel = level;
}

void LogManager::cleanOldLogs(int keepDays)
{
    emit sig_cleanOldLogs(keepDays);
}

void LogManager::debug(const QString& message, const char* file, int line, const char* function)
{
    if (m_minLogLevel > LOG_DEBUG) return;
    emit sig_writeLog(LOG_DEBUG, message, file ? QString(file) : QString(), line, function ? QString(function) : QString());
}

void LogManager::info(const QString& message, const char* file, int line, const char* function)
{
    if (m_minLogLevel > LOG_INFO) return;
    emit sig_writeLog(LOG_INFO, message, file ? QString(file) : QString(), line, function ? QString(function) : QString());
}

void LogManager::warning(const QString& message, const char* file, int line, const char* function)
{
    if (m_minLogLevel > LOG_WARNING) return;
    emit sig_writeLog(LOG_WARNING, message, file ? QString(file) : QString(), line, function ? QString(function) : QString());
}

void LogManager::error(const QString& message, const char* file, int line, const char* function)
{
    if (m_minLogLevel > LOG_ERROR) return;
    emit sig_writeLog(LOG_ERROR, message, file ? QString(file) : QString(), line, function ? QString(function) : QString());
}

void LogManager::critical(const QString& message, const char* file, int line, const char* function)
{
    emit sig_writeLog(LOG_CRITICAL, message, file ? QString(file) : QString(), line, function ? QString(function) : QString());
}

void LogManager::initWorkerThread()
{
    m_logThread = new QThread(this);
    m_logWorker = new LogWorker();
    m_logWorker->moveToThread(m_logThread);

    // 连接信号槽
    connect(this, &LogManager::sig_initLogFile, m_logWorker, &LogWorker::initLogFile);
    connect(this, &LogManager::sig_writeLog, m_logWorker, &LogWorker::writeLog);
    connect(this, &LogManager::sig_cleanOldLogs, m_logWorker, &LogWorker::cleanOldLogs);
    connect(this, &LogManager::sig_releaseAll, m_logWorker, &LogWorker::releaseAll);

    connect(m_logThread, &QThread::finished, m_logWorker, &QObject::deleteLater);
    m_logThread->start();
}