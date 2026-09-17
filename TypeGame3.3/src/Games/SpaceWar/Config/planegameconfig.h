/* ------------------------------------------------------------------
// 文件名     : planegameconfig.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 太空大战配置解析器，管理速度等级、同屏数量、分数与生命系统
------------------------------------------------------------------ */
#pragma once
#include "gameconfigbase.h"
#include "singleton.h"
#include "commondefs.h"
#include <QSize>

class PlaneGameConfig : public GameConfigBase, public Singleton<PlaneGameConfig>
{
    Q_OBJECT
        friend class Singleton<PlaneGameConfig>;
public:
    ~PlaneGameConfig() override = default;

    // ========= 显式禁用拷贝和赋值 =========
    PlaneGameConfig(const PlaneGameConfig&) = delete;
    PlaneGameConfig& operator=(const PlaneGameConfig&) = delete;
    // ===========================================

    static constexpr int MIN_PLANE_COUNT = 1;
    static constexpr int MAX_PLANE_COUNT = 10;

    // ================= 分数与玩家生命配置 =================
    int getScore() const { return m_score; }
    void addScore(int points) {
        m_score += points;
        emit scoreChanged(m_score);
    }

    bool isRewardModeEnabled() const { return m_rewardModeEnabled; }
    void setRewardModeEnabled(bool enabled) {
        if (m_rewardModeEnabled == enabled) return;
        m_rewardModeEnabled = enabled;
        emit configChanged();
    }

    // 重写重置逻辑，清空分数
    void resetStats() override {
        GameConfigBase::resetStats();
    }

    // 【新增】提供独立的分数重置接口，仅在重新开始游戏时调用
    void resetScore() {
        m_score = 0;
        emit scoreChanged(m_score);
    }

    // ========== 新增：分辨率配置 ==========
    QSize getResolution() const { return m_resolution; }
    void setResolution(const QSize& res) {
        if (m_resolution != res) {
            m_resolution = res;
            emit resolutionChanged(res.width(), res.height());
        }
    }

signals:
    void scoreChanged(int score);
    void upgradeTriggered(int newLevel); // 升级信号
    void resolutionChanged(int w, int h); // 分辨率变更信号

protected:
    bool m_rewardModeEnabled = false; // 默认关闭奖励模式

private:
    explicit PlaneGameConfig(QObject* parent = nullptr);

    int m_score = 0;
    QSize m_resolution = QSize(1366, 768); // 默认采用主流推荐分辨率
};