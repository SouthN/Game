#include "audiomanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

// ==================== AudioWorker 工作类实现 ====================
void AudioWorker::preloadAudio(AudioType type, const QString& filePath)
{
    // 已缓存则跳过（判断对象池中是否已有该类型）
    if (m_effectPool.contains(type)) return;

    QUrl audioUrl;
    if (filePath.startsWith(":/"))
    {
        audioUrl = QUrl("qrc" + filePath); 
    }
    else
    {
        audioUrl = QUrl::fromLocalFile(QDir::cleanPath(filePath));
    }

    // 区分音效和背景音乐：短音效预加载到SoundEffect，背景音乐仅缓存路径
    if (type < 100)
    {
        m_bgMusicPathMap[type] = audioUrl.toString();
        qDebug() << "BGM Load Success:" << type << audioUrl;
        return;
    }

    // ========== 【核心修改：一次性预分配对象池】 ==========
    EffectPool pool;
    for (int i = 0; i < MAX_CONCURRENT_SOUNDS; ++i) {
        QSoundEffect* effect = new QSoundEffect(this);

        // 只对第一个实例绑定日志，避免日志刷屏
        if (i == 0) {
            connect(effect, &QSoundEffect::statusChanged, this, [type, effect, audioUrl]() {
                if (effect->status() == QSoundEffect::Ready) {
                    qDebug() << "[AudioWorker] Effect Pool Load Success:" << type << audioUrl;
                }
                else if (effect->status() == QSoundEffect::Error) {
                    qWarning() << "[AudioWorker] Effect Pool Load Failed:" << type << audioUrl;
                }
                });
        }

        effect->setSource(audioUrl);
        effect->setVolume(m_effectVolume * m_masterVolume);
        pool.effects.append(effect);
    }

    // 存入哈希表
    m_effectPool[type] = pool;
}

void AudioWorker::preloadAudioBatch(const AudioTypeHash& audioMap)
{
    for (auto it = audioMap.begin(); it != audioMap.end(); ++it)
    {
        preloadAudio(it.key(), it.value());
    }
}

void AudioWorker::playBackgroundMusic(AudioType type, int loops)
{
    if (!m_bgMusicPathMap.contains(type))
    {
        qWarning() << "BGM Load Failed:" << type;
        return;
    }

    if (!m_bgPlayer)
    {
        m_bgPlayer = new QMediaPlayer(this);
        m_bgPlaylist = new QMediaPlaylist(this);
        m_bgPlayer->setPlaylist(m_bgPlaylist);
    }

    m_bgPlaylist->clear();
    // 这里直接用 URL
    m_bgPlaylist->addMedia(QUrl(m_bgMusicPathMap[type]));

    m_bgPlaylist->setPlaybackMode(loops == QMediaPlaylist::Loop ? QMediaPlaylist::Loop : QMediaPlaylist::CurrentItemOnce);
    m_bgPlayer->setVolume(m_bgmVolume * m_masterVolume * 100);
    m_bgPlayer->play();
}

void AudioWorker::pauseBackgroundMusic()
{
    if (m_bgPlayer && m_bgPlayer->state() == QMediaPlayer::PlayingState)
    {
        m_bgPlayer->pause();
    }
}

void AudioWorker::resumeBackgroundMusic()
{
    if (m_bgPlayer && m_bgPlayer->state() == QMediaPlayer::PausedState)
    {
        m_bgPlayer->play();
    }
}

void AudioWorker::stopBackgroundMusic()
{
    if (m_bgPlayer)
    {
        m_bgPlayer->stop();
        m_bgPlaylist->clear();
    }
}

void AudioWorker::playEffect(AudioType type)
{
    if (!m_effectPool.contains(type)) {
        qWarning() << "Effect Load Failed or not preloaded:" << type;
        return;
    }

    // ========== 【核心修改：O(1) 轮询调度】 ==========
    EffectPool& pool = m_effectPool[type];
    QSoundEffect* effect = pool.effects[pool.nextIndex];

    if (effect->isPlaying()) {
        effect->stop(); // 如果极速连按超过多次打断最老的，直接强制停止再播
    }

    // ========== 【核心优化：动态混音权重 (Relative Mixing)】 ==========
    // 核心思路：Qt 音量极限是 1.0。要让受击声明显，必须主动压低射击声。
    float typeVolumeWeight = 1.0f;

    if (type == AUDIO_EFFECT_MACHINE_GUN) {
        // 【压低开火声】机炮开火非常高频，降低到 40%~50% 音量，给受击声让出听觉空间
        typeVolumeWeight = 0.55f;
    }
    else if (type == AUDIO_EFFECT_HIT_ENEMY || type == AUDIO_EFFECT_HIT_ZERG || type == AUDIO_EFFECT_HIT_ALIEN) {
        typeVolumeWeight = 1.0f;
    }
    else if (type == AUDIO_EFFECT_MISSILE) {
		// 拉满导弹发射声，导弹声音不频繁，保持全音量以突出威力感
        typeVolumeWeight = 1.0f;
    }
    else if (type == AUDIO_EFFECT_EXPLOSION || type == AUDIO_EFFECT_EXPLOSION_ZERG || type == AUDIO_EFFECT_EXPLOSION_ALIEN) {
        typeVolumeWeight = 0.7f;  // 各种爆炸声防止太吵
    }
    else if (type == AUDIO_EFFECT_PLAYER_HIT) {
        typeVolumeWeight = 1.0f;  // 玩家受击必须最响，警示玩家！
    }
    else if (type == AUDIO_EFFECT_HEAL) {
        typeVolumeWeight = 1.0f;  // 正反馈回血声拉满
    }

    // 计算最终音量：音效设置板音量 * 全局主音量 * 该音效专属权重
    float finalVolume = m_effectVolume * m_masterVolume * typeVolumeWeight;

    // qBound 确保最终输入给 Qt 的音量绝对安全（0.0 ~ 1.0 之间）
    effect->setVolume(qBound(0.0f, finalVolume, 1.0f));
    // =============================================================

    effect->play();

    // 游标递增，循环复用
    pool.nextIndex = (pool.nextIndex + 1) % MAX_CONCURRENT_SOUNDS;
}

void AudioWorker::setMasterVolume(float volume)
{
    m_masterVolume = qBound(0.0f, volume, 1.0f);
    // 同步更新所有音效池中的音量
    for (auto it = m_effectPool.begin(); it != m_effectPool.end(); ++it) {
        for (QSoundEffect* effect : it.value().effects) {
            effect->setVolume(m_effectVolume * m_masterVolume);
        }
    }
    // 同步更新背景音乐音量
    if (m_bgPlayer) {
        m_bgPlayer->setVolume(m_bgmVolume * m_masterVolume * 100);
    }
}

void AudioWorker::setBGMVolume(float volume)
{
    m_bgmVolume = qBound(0.0f, volume, 1.0f);
    if (m_bgPlayer)
    {
        m_bgPlayer->setVolume(m_bgmVolume * m_masterVolume * 100);
    }
}

void AudioWorker::setEffectVolume(float volume)
{
    m_effectVolume = qBound(0.0f, volume, 1.0f);
    for (auto it = m_effectPool.begin(); it != m_effectPool.end(); ++it) {
        for (QSoundEffect* effect : it.value().effects) {
            effect->setVolume(m_effectVolume * m_masterVolume);
        }
    }
}

void AudioWorker::releaseAll()
{
    stopBackgroundMusic();
    // 释放所有对象池中的音效实例
    for (auto it = m_effectPool.begin(); it != m_effectPool.end(); ++it) {
        qDeleteAll(it.value().effects);
    }
    m_effectPool.clear();
    m_bgMusicPathMap.clear();
    // 释放播放器
    delete m_bgPlayer;
    m_bgPlayer = nullptr;
    delete m_bgPlaylist;
    m_bgPlaylist = nullptr;
    qDebug() << "All Sound Resources Release";
}

// ==================== AudioManager 管理器实现 ====================
AudioManager::AudioManager(QObject* parent)
    : QObject(parent)
{
    // 【同步修改】注册别名类型，必须在信号槽连接之前执行
    qRegisterMetaType<AudioType>();
    qRegisterMetaType<AudioTypeHash>();
    qRegisterMetaType<QMediaPlayer::Error>();

    // 创建工作线程和工作对象
    m_audioThread = new QThread(this);
    m_audioWorker = new AudioWorker();
    m_audioWorker->moveToThread(m_audioThread);

    // 连接信号槽：主线程信号 -> 工作线程槽函数
    connect(this, &AudioManager::sig_preloadAudio, m_audioWorker, &AudioWorker::preloadAudio);
    connect(this, &AudioManager::sig_preloadAudioBatch, m_audioWorker, &AudioWorker::preloadAudioBatch);
    connect(this, &AudioManager::sig_playBGM, m_audioWorker, &AudioWorker::playBackgroundMusic);
    connect(this, &AudioManager::sig_pauseBGM, m_audioWorker, &AudioWorker::pauseBackgroundMusic);
    connect(this, &AudioManager::sig_resumeBGM, m_audioWorker, &AudioWorker::resumeBackgroundMusic);
    connect(this, &AudioManager::sig_stopBGM, m_audioWorker, &AudioWorker::stopBackgroundMusic);
    connect(this, &AudioManager::sig_playEffect, m_audioWorker, &AudioWorker::playEffect);
    connect(this, &AudioManager::sig_setMasterVolume, m_audioWorker, &AudioWorker::setMasterVolume);
    connect(this, &AudioManager::sig_setBGMVolume, m_audioWorker, &AudioWorker::setBGMVolume);
    connect(this, &AudioManager::sig_setEffectVolume, m_audioWorker, &AudioWorker::setEffectVolume);
    connect(this, &AudioManager::sig_releaseAll, m_audioWorker, &AudioWorker::releaseAll);

    // 线程退出时释放工作对象
    connect(m_audioThread, &QThread::finished, m_audioWorker, &QObject::deleteLater);

    // 启动音频工作线程
    m_audioThread->start();
}

AudioManager::~AudioManager()
{
    // 安全退出线程
    sig_releaseAll();
    m_audioThread->quit();
    m_audioThread->wait(3000);
}

void AudioManager::preloadAudio(AudioType type, const QString& filePath)
{
    emit sig_preloadAudio(type, filePath);
}

void AudioManager::preloadAudioBatch(const AudioTypeHash& audioMap)
{
    emit sig_preloadAudioBatch(audioMap);
}

void AudioManager::playBackgroundMusic(AudioType type, int loops)
{
    emit sig_playBGM(type, loops);
}

void AudioManager::pauseBackgroundMusic()
{
    emit sig_pauseBGM();
}

void AudioManager::resumeBackgroundMusic()
{
    emit sig_resumeBGM();
}

void AudioManager::stopBackgroundMusic()
{
    emit sig_stopBGM();
}

void AudioManager::playEffect(AudioType type)
{
    // ========== 【核心优化：动态防抖节流】 ==========
    {
        QMutexLocker locker(&m_throttleMutex);
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        if (m_lastPlayTime.contains(type)) {
            // 默认防抖时间
            qint64 throttleMs = EFFECT_THROTTLE_MS;

            // 针对不同频率的音效，动态设定节流窗口
            if (type == AUDIO_EFFECT_MACHINE_GUN) {
                throttleMs = 100; // 机炮连发极快，100ms 允许播放一次足矣
            }
            else if (type == AUDIO_EFFECT_HIT_ENEMY) {
                throttleMs = 50;  // 击中装甲声可以密集一些，50ms
            }
            else if (type == AUDIO_EFFECT_EXPLOSION) {
                throttleMs = 80;  // 群体爆炸容易炸耳，调高到 80ms 合并音效
            }

            if (currentTime - m_lastPlayTime[type] < throttleMs) {
                return; // 距离上次播放同种音效太近，直接丢弃合并
            }
        }
        m_lastPlayTime[type] = currentTime;
    }
    // ============================================
    emit sig_playEffect(type);
}

void AudioManager::setMasterVolume(float volume)
{
    emit sig_setMasterVolume(volume);
}

void AudioManager::setBGMVolume(float volume)
{
    emit sig_setBGMVolume(volume);
}

void AudioManager::setEffectVolume(float volume)
{
    emit sig_setEffectVolume(volume);
}