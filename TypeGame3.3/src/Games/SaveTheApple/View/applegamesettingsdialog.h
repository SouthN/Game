/* ------------------------------------------------------------------
// 文件名     : applegamesettingsdialog.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果设置面板，提供用户交互界面以修改游戏难度和音效参数
------------------------------------------------------------------ */
#pragma once
#include <QDialog>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include "applegameconfig.h"
#include "commondefs.h"

class AppleGameSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AppleGameSettingsDialog(QWidget* parent = nullptr);
    ~AppleGameSettingsDialog() override = default;

    // ========= 新增：禁用拷贝和赋值 =========
    AppleGameSettingsDialog(const AppleGameSettingsDialog&) = delete;
    AppleGameSettingsDialog& operator=(const AppleGameSettingsDialog&) = delete;
    // ========================================

protected:
    // 无边框窗口拖动事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onOkClicked();
    void onCancelClicked();

private:
    void initUI();
    void loadCurrentSettings();
    void applySettings();

    // 分辨率选择框
    QComboBox* m_resolutionComboBox = nullptr; 
    // 游戏配置滑块
    QSlider* m_levelSlider = nullptr;
    QSlider* m_appleCountSlider = nullptr;
    QSlider* m_targetSlider = nullptr;
    // 数值显示标签
    QLabel* m_levelValueLabel = nullptr;
    QLabel* m_appleCountValueLabel = nullptr;
    QLabel* m_targetValueLabel = nullptr;

    // ========== 新增：音效与音量控件 ==========
    QCheckBox* m_soundEnableCheckBox = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QLabel* m_volumeValueLabel = nullptr;

    // ========== 新增：后处理参数滑块（仅需UI控件） ==========
    QSlider* m_brightnessSlider = nullptr;
    QSlider* m_contrastSlider = nullptr;
    QSlider* m_saturationSlider = nullptr;
    QSlider* m_vignetteSlider = nullptr;
    QLabel* m_brightnessValueLabel = nullptr;
    QLabel* m_contrastValueLabel = nullptr;
    QLabel* m_saturationValueLabel = nullptr;
    QLabel* m_vignetteValueLabel = nullptr;

    // ========== 新增：后处理参数控件 ==========
    QSlider* m_blurRadiusSlider = nullptr;
    QSlider* m_sharpenIntensitySlider = nullptr;
    QSlider* m_hueShiftSlider = nullptr;
    QSlider* m_grayscaleMixSlider = nullptr;
    QSlider* m_filmGrainSlider = nullptr;
    QSlider* m_scanlineIntensitySlider = nullptr;
    QSlider* m_pixelationSizeSlider = nullptr;

    QLabel* m_blurRadiusValueLabel = nullptr;
    QLabel* m_sharpenIntensityValueLabel = nullptr;
    QLabel* m_hueShiftValueLabel = nullptr;
    QLabel* m_grayscaleMixValueLabel = nullptr;
    QLabel* m_filmGrainValueLabel = nullptr;
    QLabel* m_scanlineIntensityValueLabel = nullptr;
    QLabel* m_pixelationSizeValueLabel = nullptr;


    // 新增：拖动相关变量
    bool m_isDragging = false;
    QPoint m_dragStartPos;
};