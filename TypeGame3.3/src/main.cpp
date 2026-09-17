#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "resourcemanager.h" // 引入图片资源管理器
#include "entityfactory.h"      
#include "applegameconfig.h"     
#include "planegameconfig.h"     
#include "backpackdata.h"        
#include "audiomanager.h" // 引入音乐资源管理器
#include "logmanager.h"
#include "analyticsmanager.h"
#include <QCommandLineParser>
#include "appletestrunner.h" // 引入拯救苹果无头测试运行器
#include "spacetestrunner.h" // 引入太空大战无头测试运行器

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // ========== 新增：初始化日志和埋点系统 ==========
    LogManager::instance()->init();
    LogManager::instance()->setLogLevel(LOG_DEBUG);
    LOG_INFO("Application started");

    AnalyticsManager::instance()->init();
    // 设置公共属性
    AnalyticsManager::instance()->setCommonProperty("app_version", "2.0");
    AnalyticsManager::instance()->setCommonProperty("platform", "Windows");
    // 记录应用启动事件
    AnalyticsManager::instance()->trackEvent("app_start");
    // =================================================

    // ========== 新增：命令行解析 ==========
    QCommandLineParser parser;
    parser.setApplicationDescription("TypeGame 2024 Launcher");
    parser.addHelpOption();

    QCommandLineOption testOption("test", "Run in automated test mode.");
    parser.addOption(testOption);

    QCommandLineOption inputOption("input", "Input game config file (json).", "file");
    parser.addOption(inputOption);

    QCommandLineOption outputOption("output", "Output test result file (json).", "file");
    parser.addOption(outputOption);

    parser.addPositionalArgument("game_name", "Game to test: 'apple' or 'space'.");

    parser.process(a);

    // 判断是否以 --test 模式启动
    if (parser.isSet(testOption)) {
        QStringList args = parser.positionalArguments();
        QString gameName = args.isEmpty() ? "" : args.first();
        QString inputPath = parser.value(inputOption);
        QString outputPath = parser.value(outputOption);

        int retCode = 0;
        if (gameName == "apple") {
            // 执行拯救苹果无头测试
            retCode = AppleTestRunner::runTest(inputPath, outputPath);
        }
        else if (gameName == "space") {
            // 接入太空大战的自动化命令行测试
            retCode = SpaceTestRunner::runTest(inputPath, outputPath);
        }
        else {
            qWarning() << "[Test Mode] Unknown game name:" << gameName;
            retCode = -1;
        }

        // 测试模式下，执行完毕直接退出，绝不拉起任何 UI 窗口
        // 清理单例后直接返回
        LOG_INFO("Test Mode Exited");
        AnalyticsManager::destroyInstance();
        LogManager::destroyInstance();
        EntityFactory::destroyInstance();
        AppleGameConfig::destroyInstance();
        PlaneGameConfig::destroyInstance(); // 确保太空大战单例也被销毁
        return retCode;
    }
    // ==================================================

    // 以下是正常的 GUI 启动流程 (不含 --test 时执行)，将各种耗时的初始化搬到这里
    // ========== 国际化翻译加载优化 ==========
    QTranslator translator;
    // 自动适配系统语言，也可手动指定
    //QString locale = QLocale::system().name(); // 获取系统语言（如zh_CN、en_US）
    QString qmPath = QString(":/translations/translations/zh_CN.qm");

    // 加载失败时默认使用英文
    if (!translator.load(qmPath)) {
        qWarning() << "Load Translation Fail Use Default English:" << qmPath;
        translator.load(":/translations/translations/en_US.qm");
    }
    a.installTranslator(&translator);
    // =========================================

    // 全局样式表加载（完全保留原有样式）
    QFile styleFile(":/MainWindow/assets/qss/MainWindowStyle.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        stream.setCodec("UTF-8");
        QString styleSheet = stream.readAll();
        a.setStyleSheet(styleSheet);
        styleFile.close();
    }

    // 预加载苹果图片，避免首次加载卡顿
    ResourceManager::instance()->loadPixmap(":/SaveTheApple/assets/images/SaveTheApple/APPLE_REAL.png");
    ResourceManager::instance()->loadPixmap(":/SaveTheApple/assets/images/SaveTheApple/APPLE_BAD2.png");

    // ========== 新增：音效资源预加载（游戏启动时执行，避免首次播放卡顿） ==========
    AudioTypeHash audioMap;
    // 1. 区分两个游戏的背景音乐
    audioMap[AUDIO_BG_APPLE] = ":/Audios/assets/sounds/BGM.mp3";
    audioMap[AUDIO_BG_SPACE] = ":/Audios/assets/sounds/BGM2.mp3";
    // 2. 苹果游戏基础音效
    audioMap[AUDIO_EFFECT_APPLE_HIT] = ":/Audios/assets/sounds/HitFlesh1.wav";
    audioMap[AUDIO_EFFECT_APPLE_MISS] = ":/Audios/assets/sounds/HitFlesh3.wav";
    // 3. 太空大战专属音效预加载
    audioMap[AUDIO_EFFECT_MACHINE_GUN] = ":/Audios/assets/sounds/Machinegun.wav";
    audioMap[AUDIO_EFFECT_MISSILE] = ":/Audios/assets/sounds/Bullet.wav";
    audioMap[AUDIO_EFFECT_HIT_ENEMY] = ":/Audios/assets/sounds/PlaneHit.wav";
    audioMap[AUDIO_EFFECT_EXPLOSION] = ":/Audios/assets/sounds/Explosion.wav";
    audioMap[AUDIO_EFFECT_LEVEL_UP] = ":/Audios/assets/sounds/Upgrade.wav";
    // ========== 新增：预加载各类拓展音效 ==========
    audioMap[AUDIO_EFFECT_HIT_ZERG] = ":/Audios/assets/sounds/ZergHit.wav"; // 替换为实际路径
    audioMap[AUDIO_EFFECT_EXPLOSION_ZERG] = ":/Audios/assets/sounds/ZergExplosion.wav"; // 替换为实际路径
    audioMap[AUDIO_EFFECT_HIT_ALIEN] = ":/Audios/assets/sounds/AlienHit.wav"; // 替换为实际路径
    audioMap[AUDIO_EFFECT_EXPLOSION_ALIEN] = ":/Audios/assets/sounds/AlienExplosion.wav"; // 替换为实际路径
    audioMap[AUDIO_EFFECT_PLAYER_HIT] = ":/Audios/assets/sounds/PlayerHit.wav";
    audioMap[AUDIO_EFFECT_HEAL] = ":/Audios/assets/sounds/Heal.wav";
    AudioManager::instance()->preloadAudioBatch(audioMap);
    // ==============================================================

    MainWindow w;
    w.show();
    int ret = a.exec(); // 事件循环退出后执行清理

    // ========== 新增：清理日志和埋点系统 ==========
    LOG_INFO("Application exited");
    AnalyticsManager::destroyInstance();
    LogManager::destroyInstance();
    // =================================================

    // ========== 新增：销毁音效管理器单例 ==========
    AudioManager::destroyInstance();
    // ========== 新增：销毁所有单例，释放资源 ==========
    EntityFactory::destroyInstance();
    ResourceManager::destroyInstance();
    AppleGameConfig::destroyInstance();
    PlaneGameConfig::destroyInstance(); // <--- 补充这一行，清理太空大战的单例
    BackpackData::destroyInstance();
    // ===================================================

    return ret;
}