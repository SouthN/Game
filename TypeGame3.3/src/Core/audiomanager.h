/* ------------------------------------------------------------------
// 文件名     : audiomanager.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 音频资源管理器，负责背景音乐及短促音效的预加载与播放控制
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QThread>
#include <QHash>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDateTime>
#include <QMutex>
#include "singleton.h"

// 新增：音频类型枚举
enum AudioType
{
    // 背景音乐
    AUDIO_BG_APPLE = 0,            // 拯救苹果专属背景音乐
    AUDIO_BG_SPACE = 1,            // 太空大战专属背景音乐
    AUDIO_BG_MAIN_MENU = 2,

    // 音效（短音频）
    AUDIO_EFFECT_BTN_CLICK = 100,  // 按钮点击音效
    AUDIO_EFFECT_APPLE_HIT,         // 苹果命中/选中音效
    AUDIO_EFFECT_APPLE_MISS,        // 苹果未命中/落地失败音效
    AUDIO_EFFECT_LEVEL_COMPLETE,    // 关卡完成音效
    AUDIO_EFFECT_GAME_OVER,         // 游戏结束音效

    // ========== 新增：太空大战专属音效 ==========
    AUDIO_EFFECT_MACHINE_GUN,      // 机炮射击
    AUDIO_EFFECT_MISSILE,          // 导弹发射
    AUDIO_EFFECT_HIT_ENEMY,        // 击中敌机装甲
    AUDIO_EFFECT_EXPLOSION,        // 敌机坠毁爆炸
    AUDIO_EFFECT_LEVEL_UP,          // 升级音效

    // ========== 新增：虫族阵营专属音效 ==========
    AUDIO_EFFECT_HIT_ZERG,         // 击中虫族 (飙血声)
    AUDIO_EFFECT_EXPLOSION_ZERG,    // 虫族死亡爆裂声

    // ========== 新增：外星阵营专属音效 ==========
    AUDIO_EFFECT_HIT_ALIEN,         // 击中外星 (护盾声)
    AUDIO_EFFECT_EXPLOSION_ALIEN,    // 外星死亡爆炸声

    AUDIO_EFFECT_PLAYER_HIT,       // 玩家受击/撞击音效
    AUDIO_EFFECT_HEAL              // 奖励单词回血音效
};

// ==================== 修复1：实现qHash函数，支持AudioType作为QHash的键 ====================
inline uint qHash(AudioType key, uint seed = 0)
{
    // 枚举转int，复用Qt内置的哈希函数
    return qHash(static_cast<int>(key), seed);
}

// ==================== 修复2：typedef重命名，解决宏解析逗号的问题 ====================
typedef QHash<AudioType, QString> AudioTypeHash;

// ==================== 修复3：注册元类型，使用无逗号的别名类型 ====================
Q_DECLARE_METATYPE(AudioType)
Q_DECLARE_METATYPE(AudioTypeHash)


// 音频工作类：所有音频操作均在独立线程执行，不阻塞主线程
class AudioWorker : public QObject
{
    Q_OBJECT
public slots:
    // 预加载单个音频
    void preloadAudio(AudioType type, const QString& filePath);
    // 批量预加载音频
    void preloadAudioBatch(const AudioTypeHash& audioMap);
    // 播放背景音乐（循环）
    void playBackgroundMusic(AudioType type, int loops = QMediaPlaylist::Loop);
    // 暂停背景音乐
    void pauseBackgroundMusic();
    // 恢复背景音乐
    void resumeBackgroundMusic();
    // 停止背景音乐
    void stopBackgroundMusic();
    // 播放短音效
    void playEffect(AudioType type);
    // 设置全局音量（0.0-1.0）
    void setMasterVolume(float volume);
    // 设置背景音乐音量（0.0-1.0）
    void setBGMVolume(float volume);
    // 设置音效音量（0.0-1.0）
    void setEffectVolume(float volume);
    // 释放所有音频资源
    void releaseAll();

private:
    // ========== 【终极修复：预分配音效池与轮询调度】 ==========
    struct EffectPool {
        QVector<QSoundEffect*> effects;
        int nextIndex = 0; // 轮询游标
    };
    QHash<AudioType, EffectPool> m_effectPool;
    static constexpr int MAX_CONCURRENT_SOUNDS = 15; // 每种音效预先创建 15 个实例
    // ==================================================
    QHash<AudioType, QString> m_bgMusicPathMap;       // 背景音乐路径缓存
    QMediaPlayer* m_bgPlayer = nullptr;                // 背景音乐播放器
    QMediaPlaylist* m_bgPlaylist = nullptr;            // 背景音乐播放列表
    float m_masterVolume = 1.0f;                       // 全局音量
    float m_bgmVolume = 0.7f;                          // 背景音乐默认音量
    float m_effectVolume = 1.0f;                       // 音效默认音量
};

// 音效管理器：主线程调用入口，单例模式，线程安全
class AudioManager : public QObject, public Singleton<AudioManager>
{
    Q_OBJECT
        friend class Singleton<AudioManager>;
public:
    ~AudioManager() override;

    // ========= 新增：显式禁用拷贝和赋值 =========
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    // ===========================================

    // 预加载音频（主线程调用，异步执行）
    void preloadAudio(AudioType type, const QString& filePath);
    void preloadAudioBatch(const AudioTypeHash& audioMap);

    // 背景音乐控制
    void playBackgroundMusic(AudioType type, int loops = QMediaPlaylist::Loop);
    void pauseBackgroundMusic();
    void resumeBackgroundMusic();
    void stopBackgroundMusic();

    // 音效播放
    void playEffect(AudioType type);

    // 音量控制
    void setMasterVolume(float volume);
    void setBGMVolume(float volume);
    void setEffectVolume(float volume);

signals:
    // 信号：转发到工作线程的槽函数
    void sig_preloadAudio(AudioType type, const QString& filePath);
    void sig_preloadAudioBatch(const AudioTypeHash& audioMap);
    void sig_playBGM(AudioType type, int loops);
    void sig_pauseBGM();
    void sig_resumeBGM();
    void sig_stopBGM();
    void sig_playEffect(AudioType type);
    void sig_setMasterVolume(float volume);
    void sig_setBGMVolume(float volume);
    void sig_setEffectVolume(float volume);
    void sig_releaseAll();

private:
    explicit AudioManager(QObject* parent = nullptr);
    QThread* m_audioThread = nullptr;
    AudioWorker* m_audioWorker = nullptr;

    // ========== 【核心优化：音效防抖节流记录】 ==========
    QHash<AudioType, qint64> m_lastPlayTime;
    QMutex m_throttleMutex;
    static constexpr qint64 EFFECT_THROTTLE_MS = 30; // 30ms 防抖窗口
};