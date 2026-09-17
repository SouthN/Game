#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include "singleton.h"

class WordGenerator : public QObject, public Singleton<WordGenerator>
{
    Q_OBJECT
        friend class Singleton<WordGenerator>;
public:
    ~WordGenerator() override = default;

    // 发起异步请求
    void requestWordAsync();

signals:
    // 单词生成完毕（无论是API返回还是降级本地词库）
    void wordGenerated(const QString& word);

private:
    explicit WordGenerator(QObject* parent = nullptr);
    void handleNetworkReply(QNetworkReply* reply);
    QString getFallbackWord();

    QNetworkAccessManager* m_networkManager = nullptr;
    QStringList m_fallbackDictionary; // 本地降级词库
    bool m_isRequesting = false;
};