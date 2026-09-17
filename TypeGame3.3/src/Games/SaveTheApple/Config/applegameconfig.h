/* ------------------------------------------------------------------
// 文件名     : applegameconfig.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果配置解析器，管理速度等级、同屏数量及过关目标等设置项
------------------------------------------------------------------ */
#pragma once
#include "gameconfigbase.h"
#include "singleton.h"
#include "commondefs.h" 
#include <QSize>

class AppleGameConfig : public GameConfigBase, public Singleton<AppleGameConfig>
{
    Q_OBJECT
    friend class Singleton<AppleGameConfig>;
public:
    ~AppleGameConfig() override = default;

    // ========= 新增：显式禁用拷贝和赋值 =========
    AppleGameConfig(const AppleGameConfig&) = delete;
    AppleGameConfig& operator=(const AppleGameConfig&) = delete;
    // ===========================================

    static constexpr int MIN_APPLE_COUNT = 1;
    static constexpr int MAX_APPLE_COUNT = 5;

    // ========== 新增：分辨率配置 ==========
    QSize getResolution() const { return m_resolution; }
    void setResolution(const QSize& res) {
        if (m_resolution != res) {
            m_resolution = res;
            emit resolutionChanged(res.width(), res.height());
        }
    }

signals:
    void resolutionChanged(int w, int h); // 分辨率变更信号

private:
    explicit AppleGameConfig(QObject* parent = nullptr);
    QSize m_resolution = QSize(1366, 768); // 默认采用主流推荐分辨率
};