/* ------------------------------------------------------------------
// 文件名     : spriteanimation.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 精灵序列帧动画处理类，实现简单的 2D 帧动画播放与控制
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QPixmap>
#include <QVector>
#include <QTimer>

class SpriteAnimation : public QObject
{
    Q_OBJECT
public:
    explicit SpriteAnimation(QObject* parent = nullptr);
    ~SpriteAnimation() override = default;

    // 动画配置
    void addFrame(const QPixmap& frame);
    void setFrameInterval(int ms);
    void setLoop(bool loop);

    // 动画控制
    void start();
    void stop();
    void reset();

    // 获取当前帧
    QPixmap getCurrentFrame() const;
    bool isPlaying() const;

signals:
    void frameChanged(int frameIndex);
    void animationFinished();

private slots:
    void onFrameTimerTimeout();

private:
    QVector<QPixmap> m_frames;
    int m_currentFrameIndex = 0;
    int m_frameInterval = 50;
    bool m_isLoop = true;
    bool m_isPlaying = false;
    QTimer* m_frameTimer = nullptr;
};