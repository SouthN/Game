#include "wordgenerator.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QTimer>
#include "logmanager.h"

WordGenerator::WordGenerator(QObject* parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);

    // 初始化本地降级词库 (扩充为通用词汇，涵盖生活、自然、动物等)
    m_fallbackDictionary << "GALAXY" << "TIGER" << "APPLE" << "OCEAN"
        << "MAGIC" << "RIVER" << "SWORD" << "EAGLE"
        << "STORM" << "FLAME" << "CLOUD" << "PIANO"
        << "BREAD" << "TRAIN" << "WATCH" << "SMILE";
}

void WordGenerator::requestWordAsync()
{
    if (m_isRequesting) return;
    m_isRequesting = true;

    // 此处使用 OpenAI 兼容的 API 作为演示。实际生产可替换为通义/文心等 endpoint
    QUrl url("https://api.deepseek.com/v1/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // 注意：实际项目中不要把 Token 硬编码在这里，应从配置或环境变量读取
    //request.setRawHeader("Authorization", "Bearer sk-6773801176f4402395f4dd0fa75d753c");
    request.setRawHeader("Authorization", "Bearer");

    QJsonObject message;
    message["role"] = "user";

    // ========== 【终极随机化：随机主题 + 随机首字母】 ==========
    // 1. 定义一个丰富的随机主题库
    QStringList themes = {
        "animals", "nature", "food", "emotions", "weather",
        "colors", "sports", "music", "clothing", "technology",
        "magic", "ocean", "plants", "jobs", "travel"
    };
    int themeIdx = QRandomGenerator::global()->bounded(themes.size());
    QString targetTheme = themes.at(themeIdx);

    // 2. 依然保留随机首字母，双重限制打破大模型惯性
    QString availableLetters = "ABCDEFGHIJKLMNOPRSTUVW";
    int letterIdx = QRandomGenerator::global()->bounded(availableLetters.length());
    QChar targetLetter = availableLetters.at(letterIdx);

    // 3. 动态拼接终极 Prompt
    QString dynamicPrompt = QString(
        "Output exactly one common English word related to the theme of '%1'. "
        "IMPORTANT: The word MUST start with the letter '%2'. "
        "Length: 4 to 8 letters. No punctuation, no extra text."
    ).arg(targetTheme).arg(targetLetter);

    message["content"] = dynamicPrompt;

    QJsonArray messages;
    messages.append(message);

    QJsonObject json;
    json["model"] = "deepseek-chat"; // 或其他大模型
    json["messages"] = messages;
	json["temperature"] = 1.1; // 你还可以适当把 temperature 调高一点，比如 1.0 或 1.2，增加模型回答的随机性

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(json).toJson());

    // 设置超时机制 (3秒)，防止 API 假死导致游戏不出词
    QTimer::singleShot(3000, reply, [reply, this]() {
        if (reply->isRunning()) {
            reply->abort(); // 将触发 error 信号进而走降级逻辑
        }
        });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleNetworkReply(reply);
        });
}

void WordGenerator::handleNetworkReply(QNetworkReply* reply)
{
    m_isRequesting = false;
    QString finalWord;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        if (obj.contains("choices")) {
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject msgObj = choices[0].toObject()["message"].toObject();
                finalWord = msgObj["content"].toString().trimmed().toUpper();
            }
        }
    }

    reply->deleteLater();

    // 格式校验与降级验证：包含非字母、空、或长度异常，均走本地词库降级
    bool isValid = true;
    for (QChar c : finalWord) {
        if (!c.isLetter()) isValid = false;
    }

    if (finalWord.isEmpty() || finalWord.length() < 3 || finalWord.length() > 10 || !isValid) {
        LOG_WARNING("LLM API failed or invalid response. Triggering fallback dictionary.");
        finalWord = getFallbackWord();
    }
    else {
        LOG_INFO(QString("LLM API generated reward word: %1").arg(finalWord));
    }

    emit wordGenerated(finalWord);
}

QString WordGenerator::getFallbackWord()
{
    int idx = QRandomGenerator::global()->bounded(m_fallbackDictionary.size());
    return m_fallbackDictionary[idx];
}