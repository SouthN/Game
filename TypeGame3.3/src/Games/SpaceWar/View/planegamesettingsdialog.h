/* ------------------------------------------------------------------
// 文件名     : planegamesettingsdialog.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 太空大战设置面板，提供用户交互界面以修改游戏难度和音效参数
------------------------------------------------------------------ */
#pragma once
#include <QDialog>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include "planegameconfig.h"
#include "commondefs.h"

class PlaneGameSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PlaneGameSettingsDialog(QWidget* parent = nullptr);
    ~PlaneGameSettingsDialog() override = default;

    PlaneGameSettingsDialog(const PlaneGameSettingsDialog&) = delete;
    PlaneGameSettingsDialog& operator=(const PlaneGameSettingsDialog&) = delete;

protected:
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

    QSlider* m_levelSlider = nullptr;
    QSlider* m_planeCountSlider = nullptr;
    QSlider* m_targetSlider = nullptr;
    QLabel* m_levelValueLabel = nullptr;
    QLabel* m_planeCountValueLabel = nullptr;
    QLabel* m_targetValueLabel = nullptr;

    QCheckBox* m_soundEnableCheckBox = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QLabel* m_volumeValueLabel = nullptr;

    QCheckBox* m_rewardModeCheckBox = nullptr; // 奖励模式开关

    QSlider* m_brightnessSlider = nullptr;
    QSlider* m_contrastSlider = nullptr;
    QSlider* m_saturationSlider = nullptr;
    QSlider* m_vignetteSlider = nullptr;
    QLabel* m_brightnessValueLabel = nullptr;
    QLabel* m_contrastValueLabel = nullptr;
    QLabel* m_saturationValueLabel = nullptr;
    QLabel* m_vignetteValueLabel = nullptr;

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

    bool m_isDragging = false;
	QPoint m_dragStartPos; // 鼠标拖动开始位置
};