/* ------------------------------------------------------------------
// 文件名     : gameconfigbase.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 游戏配置基类，提供统一的配置读取和写入接口规范
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <cmath> // 新增：用于fmod模数计算
#include "commondefs.h" // 引入基础定义

class GameConfigBase : public QObject
{
    Q_OBJECT
public:
    explicit GameConfigBase(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~GameConfigBase() = default;

    // ========= 新增：禁用拷贝和赋值 =========
    GameConfigBase(const GameConfigBase&) = delete;
    GameConfigBase& operator=(const GameConfigBase&) = delete;

    // ========== 抽取：通用统计重置 ==========
    virtual void resetStats() {
        m_successCount = 0;
        m_failCount = 0;
        emit statsChanged(m_successCount, m_failCount);
    }

    // ========== 抽取：通用等级与速度控制 ==========
    virtual int getLevel() const {
        return m_level;
    }

    virtual void setLevel(int level) {
        if (level < GameCommon::MIN_LEVEL || level > GameCommon::MAX_LEVEL) return;
        if (m_level != level) {
            m_level = level;
            emit levelChanged(m_level);
        }
    }

    virtual float getFallSpeed() const {
        // 基础速度为1，每升一级增加0.5
        return 1.0f + (m_level - 1) * 0.5f;
    }

    // ========== 新增：共性配置上浮 (过关目标与实体数量) ==========
    int getPassTarget() const { return m_passTarget; }
    void setPassTarget(int target) {
        if (m_passTarget == target) return;
        m_passTarget = target;
        emit configChanged();
    }
    int getMaxEntityCount() const { return m_maxEntityCount; }
    void setMaxEntityCount(int count) {
        if (m_maxEntityCount == count) return;
        m_maxEntityCount = count;
        emit configChanged();
    }

    // ========== 新增：共性统计数据上浮 (成功与失败计数) ==========
    int getSuccessCount() const { return m_successCount; }
    void addSuccessCount() {
        m_successCount++;
        emit successAdded();
        emit statsChanged(m_successCount, m_failCount);
    }
    int getFailCount() const { return m_failCount; }
    void addFailCount() {
        m_failCount++;
        emit failAdded();
        emit statsChanged(m_successCount, m_failCount);
    }

    // ========== 新增：音效开关与音量设置 ==========
    bool isSoundEnabled() const { return m_soundEnabled; }
    void setSoundEnabled(bool enabled) {
        if (m_soundEnabled == enabled) return;
        m_soundEnabled = enabled;
        emit soundEnabledChanged(m_soundEnabled);
        emit configChanged();
    }
    float getVolume() const { return m_volume; }
    void setVolume(float volume) {
        if (qFuzzyCompare(m_volume, volume)) return;
        m_volume = qBound(0.0f, volume, 1.0f);
        emit volumeChanged(m_volume);
        emit configChanged();
    }

    // ========== 新增：后处理参数统一接口 ==========
    float getBrightness() const { return m_brightness; }
    void setBrightness(float value) {
        if (qFuzzyCompare(m_brightness, value)) return;
        m_brightness = qBound(-1.0f, value, 1.0f);
        emit brightnessChanged(m_brightness);
        emit configChanged();
    }
    float getContrast() const { return m_contrast; }
    void setContrast(float value) {
        if (qFuzzyCompare(m_contrast, value)) return;
        m_contrast = qBound(0.0f, value, 3.0f);
        emit contrastChanged(m_contrast);
        emit configChanged();
    }
    float getSaturation() const { return m_saturation; }
    void setSaturation(float value) {
        if (qFuzzyCompare(m_saturation, value)) return;
        m_saturation = qBound(0.0f, value, 3.0f);
        emit saturationChanged(m_saturation);
        emit configChanged();
    }
    float getVignette() const { return m_vignette; }
    void setVignette(float value) {
        if (qFuzzyCompare(m_vignette, value)) return;
        m_vignette = qBound(0.0f, value, 5.0f);
        emit vignetteChanged(m_vignette);
        emit configChanged();
    }

    // ========== 新增：完整后处理参数get/set方法（带信号+去重+范围限制） ==========
    // 高斯模糊半径
    float getBlurRadius() const { return m_blurRadius; }
    void setBlurRadius(float value) {
        if (qFuzzyCompare(m_blurRadius, value)) return;
        m_blurRadius = qBound(0.0f, value, 10.0f);
        emit blurRadiusChanged(m_blurRadius);
        emit configChanged();
    }
    // 锐化强度
    float getSharpenIntensity() const { return m_sharpenIntensity; }
    void setSharpenIntensity(float value) {
        if (qFuzzyCompare(m_sharpenIntensity, value)) return;
        m_sharpenIntensity = qBound(0.0f, value, 2.0f);
        emit sharpenIntensityChanged(m_sharpenIntensity);
        emit configChanged();
    }
    // 色调偏移
    float getHueShift() const { return m_hueShift; }
    void setHueShift(float value) {
        if (qFuzzyCompare(m_hueShift, value)) return;
        m_hueShift = fmod(value, 360.0f);
        if (m_hueShift < 0) m_hueShift += 360.0f; // 保证数值在0-360之间
        emit hueShiftChanged(m_hueShift);
        emit configChanged();
    }
    // 灰度混合
    float getGrayscaleMix() const { return m_grayscaleMix; }
    void setGrayscaleMix(float value) {
        if (qFuzzyCompare(m_grayscaleMix, value)) return;
        m_grayscaleMix = qBound(0.0f, value, 1.0f);
        emit grayscaleMixChanged(m_grayscaleMix);
        emit configChanged();
    }
    // 胶片颗粒强度
    float getFilmGrain() const { return m_filmGrain; }
    void setFilmGrain(float value) {
        if (qFuzzyCompare(m_filmGrain, value)) return;
        m_filmGrain = qBound(0.0f, value, 0.2f);
        emit filmGrainChanged(m_filmGrain);
        emit configChanged();
    }
    // 扫描线强度
    float getScanlineIntensity() const { return m_scanlineIntensity; }
    void setScanlineIntensity(float value) {
        if (qFuzzyCompare(m_scanlineIntensity, value)) return;
        m_scanlineIntensity = qBound(0.0f, value, 1.0f);
        emit scanlineIntensityChanged(m_scanlineIntensity);
        emit configChanged();
    }
    // 像素化块大小
    float getPixelationSize() const { return m_pixelationSize; }
    void setPixelationSize(float value) {
        if (qFuzzyCompare(m_pixelationSize, value)) return;
        m_pixelationSize = qBound(1.0f, value, 50.0f);
        emit pixelationSizeChanged(m_pixelationSize);
        emit configChanged();
    }

signals:
    void levelChanged(int level);
	void statsChanged(int success, int fail);  // 这个信号用于通知统计数据发生变化，包含成功和失败的次数，方便界面更新和数据分析
    void configChanged();
    
    // ========== 新增：成功/失败信号上浮 ==========
    void successAdded();
    void failAdded();

    // ========== 新增：音效开关与音量设置信号 ==========
    void soundEnabledChanged(bool enabled);
    void volumeChanged(float volume);

    // ========== 新增：后处理参数变化信号 ==========
    void brightnessChanged(float value);
    void contrastChanged(float value);
    void saturationChanged(float value);
    void vignetteChanged(float value);

    // ========== 新增：后处理参数变化信号 ==========
    void blurRadiusChanged(float value);
    void sharpenIntensityChanged(float value);
    void hueShiftChanged(float value);
    void grayscaleMixChanged(float value);
    void filmGrainChanged(float value);
    void scanlineIntensityChanged(float value);
    void pixelationSizeChanged(float value);

protected:
    // ========== 新增：共性配置变量上浮 ==========
	int m_level = 1;  // 默认关卡等级
    int m_passTarget = 20;      // 默认过关目标
    int m_maxEntityCount = 3;   // 默认同屏最大实体数
    int m_successCount = 0;
    int m_failCount = 0;

    // ========== 新增：音效开关与音量 ==========
    bool m_soundEnabled = true;     // 默认开启音效
    float m_volume = 1.0f;          // 默认音量 100%

    // ========== 后处理参数默认值 ==========
    float m_brightness = 0.0f;
    float m_contrast = 1.0f;
    float m_saturation = 1.0f;
    float m_vignette = 0.4f;

    // 新增后处理参数
    float m_blurRadius = 0.0f;          // 高斯模糊半径
    float m_sharpenIntensity = 0.0f;    // 锐化强度
    float m_hueShift = 0.0f;            // 色调偏移
    float m_grayscaleMix = 0.0f;        // 灰度混合
    float m_filmGrain = 0.0f;           // 胶片颗粒
    float m_scanlineIntensity = 0.0f;   // 扫描线强度
    float m_pixelationSize = 1.0f;      // 像素化大小

};