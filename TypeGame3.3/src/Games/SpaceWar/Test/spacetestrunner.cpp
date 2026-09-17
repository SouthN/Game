/* ------------------------------------------------------------------
// 文件名     : spacetestrunner.cpp
// 功能描述   : 太空大战自动化命令行测试运行器实现
------------------------------------------------------------------ */
#include "spacetestrunner.h"
#include "planegamedata.h"
#include "planegamecontroller.h"
#include "planegameconfig.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QKeyEvent>

struct MockInput {
    int timeMs;
    QString key;
};

int SpaceTestRunner::runTest(const QString& inputJsonPath, const QString& outputJsonPath)
{
    qDebug() << "========================================";
    qDebug() << "[Space Test Runner] Starting Automated Synchronous Test...";

    // 1. 读取并解析输入配置 JSON
    QFile inFile(inputJsonPath);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qCritical() << "[Space Test Runner] Error: Cannot open input file:" << inputJsonPath;
        return -1;
    }
    QJsonObject inputObj = QJsonDocument::fromJson(inFile.readAll()).object();
    inFile.close();

    // 2. 配置初始化
    PlaneGameConfig* config = PlaneGameConfig::instance();
    if (inputObj.contains("targetScore")) config->setPassTarget(inputObj["targetScore"].toInt());
    if (inputObj.contains("level")) config->setLevel(inputObj["level"].toInt());
    if (inputObj.contains("maxPlaneCount")) config->setMaxEntityCount(inputObj["maxPlaneCount"].toInt());

    // 3. 构建无头 (Headless) MVC
    PlaneGameData* gameData = new PlaneGameData(config);
    // 视图层传入 nullptr，依靠 Controller 内部的判空保护脱离渲染管线
    PlaneGameController* controller = new PlaneGameController(gameData, nullptr);

    // 开启测试模式，屏蔽真实定时器
    controller->setTestMode(true);

    // 解析按键输入序列
    QList<MockInput> pendingInputs;
    if (inputObj.contains("mockInputs")) {
        QJsonArray inputs = inputObj["mockInputs"].toArray();
        for (const QJsonValue& val : inputs) {
            QJsonObject keyEventObj = val.toObject();
            pendingInputs.append({ keyEventObj["time_ms"].toInt(), keyEventObj["key"].toString() });
        }
    }
    std::sort(pendingInputs.begin(), pendingInputs.end(), [](const MockInput& a, const MockInput& b) {
        return a.timeMs < b.timeMs;
        });

    // 启动游戏
    controller->startGame();

    int currentTimeMs = 0;
    const int TICK_INTERVAL = 16;
    const int MAX_TEST_DURATION = 120000;
    qDebug() << "[Space Test Runner] Executing tick loop...";

    while (gameData->getGameState() == GameDataBase::Playing && currentTimeMs < MAX_TEST_DURATION) {
        // 触发按键事件
        while (!pendingInputs.isEmpty() && pendingInputs.first().timeMs <= currentTimeMs) {
            QString keyStr = pendingInputs.first().key;
            int keyCode = keyStr.isEmpty() ? 0 : keyStr[0].toUpper().unicode();
            QKeyEvent keyEvent(QEvent::KeyPress, keyCode, Qt::NoModifier, keyStr);
            controller->handleKeyPress(&keyEvent);
            pendingInputs.removeFirst();
        }

        // 驱动一帧游戏物理和逻辑
        controller->manualTick();

        currentTimeMs += TICK_INTERVAL;
    }

    if (gameData->getGameState() == GameDataBase::Playing) {
        qWarning() << "[Space Test Runner] Watchdog Timeout! Forcing GameOver...";
        gameData->setGameState(GameDataBase::GameOver);
    }

    // 4. 同步收集并写入结果
    qDebug() << "[Space Test Runner] Game Finished. Writing results...";
    int successCount = config->getSuccessCount();
    int failCount = config->getFailCount();
    int targetCount = config->getPassTarget();
    int finalScore = config->getScore();

    QJsonObject outputObj;
    outputObj["success"] = (successCount >= targetCount);
    outputObj["targetScore"] = targetCount;
    outputObj["successCount"] = successCount;
    outputObj["failCount"] = failCount;
    outputObj["finalScore"] = finalScore;
    outputObj["simulatedTimeMs"] = currentTimeMs;

    QFile outFile(outputJsonPath);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(QJsonDocument(outputObj).toJson());
        outFile.close();
        qDebug() << "[Space Test Runner] Result successfully saved to:" << outputJsonPath;
    }

    qDebug() << "========================================";

    delete controller;
    delete gameData;

    return 0;
}