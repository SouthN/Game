#include "spriteanimation.h"

SpriteAnimation::SpriteAnimation(QObject* parent)
    : QObject(parent)
{
    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &SpriteAnimation::onFrameTimerTimeout);
}

void SpriteAnimation::addFrame(const QPixmap& frame)
{
    m_frames.append(frame);
}

void SpriteAnimation::setFrameInterval(int ms)
{
    m_frameInterval = ms;
    m_frameTimer->setInterval(m_frameInterval);
}

void SpriteAnimation::setLoop(bool loop)
{
    m_isLoop = loop;
}

void SpriteAnimation::start()
{
    if (m_frames.isEmpty() || m_isPlaying) return;
    m_isPlaying = true;
    m_frameTimer->start(m_frameInterval);
}

void SpriteAnimation::stop()
{
    m_isPlaying = false;
    m_frameTimer->stop();
}

void SpriteAnimation::reset()
{
    stop();
    m_currentFrameIndex = 0;
}

QPixmap SpriteAnimation::getCurrentFrame() const
{
    if (m_frames.isEmpty()) return QPixmap();
    return m_frames[m_currentFrameIndex];
}

bool SpriteAnimation::isPlaying() const
{
    return m_isPlaying;
}

void SpriteAnimation::onFrameTimerTimeout()
{
    if (m_frames.isEmpty()) return;

    m_currentFrameIndex++;
    if (m_currentFrameIndex >= m_frames.size()) {
        if (m_isLoop) {
            m_currentFrameIndex = 0;
        } else {
            stop();
            emit animationFinished();
            return;
        }
    }

    emit frameChanged(m_currentFrameIndex);
}