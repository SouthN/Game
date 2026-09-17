/* ------------------------------------------------------------------
// 文件名     : appletestrunner.cpp
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果自动化命令行测试运行器实现
------------------------------------------------------------------ */
#include "appletestrunner.h"
#include "applegamedata.h"
#include "applegamecontroller.h"
#include "applegameconfig.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QKeyEvent>

// 用于解析输入的辅助结构体
struct MockInput {
    int timeMs;
    QString key;
};

int AppleTestRunner::runTest(const QString& inputJsonPath, const QString& outputJsonPath)
{
    qDebug() << "========================================";
    qDebug() << "[Test Runner] Starting Automated Synchronous Test...";
    
    // 1. 读取并解析输入配置 JSON
    QFile inFile(inputJsonPath);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qCritical() << "[Test Runner] Error: Cannot open input file:" << inputJsonPath;
        return -1;
    }
    QJsonObject inputObj = QJsonDocument::fromJson(inFile.readAll()).object();
    inFile.close();

    // 2. 第二阶段重构：全面独立配置解析
    AppleGameConfig* config = AppleGameConfig::instance();
    if (inputObj.contains("targetScore")) config->setPassTarget(inputObj["targetScore"].toInt());
    if (inputObj.contains("level")) config->setLevel(inputObj["level"].toInt());
    if (inputObj.contains("maxAppleCount")) config->setMaxEntityCount(inputObj["maxAppleCount"].toInt());
    
    // 3. 构建无头 (Headless) MVC
    // 修改核心：因为我们要异步执行测试，组件必须用 new 分配在堆上，否则函数一返回它们就被销毁了
    AppleGameData* gameData = new AppleGameData(config);
    AppleGameController* controller = new AppleGameController(gameData, nullptr);

    // 【核心 1】：开启测试模式，屏蔽真实定时器
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
    // 按触发时间排序（确保时序正确）
    std::sort(pendingInputs.begin(), pendingInputs.end(), [](const MockInput& a, const MockInput& b) {
        return a.timeMs < b.timeMs;
        });

    // 启动游戏逻辑
    controller->startGame();
    // 【核心 2】：手动时钟循环（瞬间完成）
    int currentTimeMs = 0;
    const int TICK_INTERVAL = 16; // 对应 GAME_LOOP_INTERVAL
    const int MAX_TEST_DURATION = 120000; // 设置 120 秒超时阈值（防止配置错误导致死循环）
    qDebug() << "[Test Runner] Executing tick loop...";

    while (gameData->getGameState() == GameDataBase::Playing && currentTimeMs < MAX_TEST_DURATION) {
        // 1. 检查当前时刻是否有按键事件需要触发
        while (!pendingInputs.isEmpty() && pendingInputs.first().timeMs <= currentTimeMs) {
            QString keyStr = pendingInputs.first().key;
            int keyCode = keyStr.isEmpty() ? 0 : keyStr[0].toUpper().unicode();
            QKeyEvent keyEvent(QEvent::KeyPress, keyCode, Qt::NoModifier, keyStr);
            controller->handleKeyPress(&keyEvent);
            pendingInputs.removeFirst();
        }

        // 2. 驱动一帧游戏逻辑（苹果下落、生成、判定）
        controller->manualTick();

        // 3. 时间流逝
        currentTimeMs += TICK_INTERVAL;
    }

    // 检查超时看门狗
    if (gameData->getGameState() == GameDataBase::Playing) {
        qWarning() << "[Test Runner] Watchdog Timeout! Forcing GameOver...";
        gameData->setGameState(GameDataBase::GameOver);
    }

    // 【核心 3】：测试结束，同步收集并写入结果
    qDebug() << "[Test Runner] Game Finished. Writing results...";
    int successCount = config->getSuccessCount();
    int failCount = config->getFailCount();
    int targetCount = config->getPassTarget();

    QJsonObject outputObj;
    outputObj["success"] = (successCount >= targetCount);
    outputObj["targetScore"] = targetCount;
    outputObj["successCount"] = successCount;
    outputObj["failCount"] = failCount;
    outputObj["simulatedTimeMs"] = currentTimeMs; // 写入模拟消耗的时间，而不是真实的物理时间

    QFile outFile(outputJsonPath);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(QJsonDocument(outputObj).toJson());
        outFile.close();
        qDebug() << "[Test Runner] Result successfully saved to:" << outputJsonPath;
    }

    qDebug() << "========================================";

    // 清理内存
    delete controller;
    delete gameData;
   
    // 直接返回 0，无需再发送退出信号
    return 0;
}