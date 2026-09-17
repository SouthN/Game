#include "applegamesettingsdialog.h"
#include "audiomanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMouseEvent> // 新增：鼠标事件头文件
#include <QFrame>

// 浮点参数与滑块整数的转换系数（局部宏，仅本文件使用）
#define BRIGHTNESS_OFFSET 100
#define BRIGHTNESS_SCALE 100.0f
#define CONTRAST_SCALE 100.0f
#define SATURATION_SCALE 100.0f
#define VIGNETTE_SCALE 100.0f

// ========== 新增：后处理参数转换宏 ==========
#define BLUR_SCALE 10.0f          // 模糊：滑块0-100 → 实际0-10
#define SHARPEN_SCALE 100.0f      // 锐化：滑块0-200 → 实际0-2
#define HUE_SCALE 1.0f             // 色调：滑块0-360 → 实际0-360
#define GRAYSCALE_SCALE 100.0f     // 灰度：滑块0-100 → 实际0-1
#define GRAIN_SCALE 1000.0f        // 颗粒：滑块0-200 → 实际0-0.2
#define SCANLINE_SCALE 100.0f      // 扫描线：滑块0-100 → 实际0-1
#define PIXELATION_SCALE 1.0f      // 像素化：滑块1-50 → 实际1-50

AppleGameSettingsDialog::AppleGameSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    // 核心：无边框设置，移除顶部标题栏，同时保留对话框基础属性
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    // 【修复1】调整对话框高度，适配新增的后处理滑块
    setFixedSize(480, 950);
    initUI();
    loadCurrentSettings();
}

void AppleGameSettingsDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void AppleGameSettingsDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void AppleGameSettingsDialog::mouseReleaseEvent(QMouseEvent* event)
{
    m_isDragging = false;
    QDialog::mouseReleaseEvent(event);
}

void AppleGameSettingsDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(16);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // ========== 新增：0. 分辨率设置 (置于第一行) ========
    QLabel* resLabel = new QLabel(tr("settings_resolution"), this);
    m_resolutionComboBox = new QComboBox(this);
    m_resolutionComboBox->addItems({
        tr("1366 * 768"),
        tr("1440 * 900"),
        tr("1440 * 1050"),
        tr("1600 * 1200"),
        tr("1680 * 1050")
        });
    formLayout->addRow(resLabel, m_resolutionComboBox);
    // ============================================

    // 1. 游戏等级滑动条
    QLabel* levelLabel = new QLabel(tr("settings_game_level"), this);
    m_levelSlider = new QSlider(Qt::Horizontal, this);
    m_levelSlider->setRange(GameCommon::MIN_LEVEL, GameCommon::MAX_LEVEL);
    m_levelSlider->setTickPosition(QSlider::TicksBelow);
    m_levelSlider->setTickInterval(1);
    m_levelValueLabel = new QLabel(QString::number(m_levelSlider->value()), this);
    m_levelValueLabel->setMinimumWidth(30);
    m_levelValueLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    QHBoxLayout* levelLayout = new QHBoxLayout();
    levelLayout->addWidget(m_levelSlider, 1);
    levelLayout->addWidget(m_levelValueLabel);
    formLayout->addRow(levelLabel, levelLayout);

    // 数值实时更新
    connect(m_levelSlider, &QSlider::valueChanged, m_levelValueLabel, [this](int value) {
        m_levelValueLabel->setText(QString::number(value));
        });

    // 2. 最大苹果数量滑动条
    QLabel* appleCountLabel = new QLabel(tr("settings_max_apple_count"), this);
    m_appleCountSlider = new QSlider(Qt::Horizontal, this);
    m_appleCountSlider->setRange(AppleGameConfig::MIN_APPLE_COUNT, AppleGameConfig::MAX_APPLE_COUNT);
    m_appleCountSlider->setTickPosition(QSlider::TicksBelow);
    m_appleCountSlider->setTickInterval(1);
    m_appleCountValueLabel = new QLabel(QString::number(m_appleCountSlider->value()), this);
    m_appleCountValueLabel->setMinimumWidth(30);
    m_appleCountValueLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    QHBoxLayout* appleCountLayout = new QHBoxLayout();
    appleCountLayout->addWidget(m_appleCountSlider, 1);
    appleCountLayout->addWidget(m_appleCountValueLabel);
    formLayout->addRow(appleCountLabel, appleCountLayout);

    // 数值实时更新
    connect(m_appleCountSlider, &QSlider::valueChanged, m_appleCountValueLabel, [this](int value) {
        m_appleCountValueLabel->setText(QString::number(value));
        });

    // 3. 通关目标数量滑动条
    QLabel* targetLabel = new QLabel(tr("settings_pass_target_count"), this);
    m_targetSlider = new QSlider(Qt::Horizontal, this);
    m_targetSlider->setRange(1, 100);
    m_targetSlider->setTickPosition(QSlider::TicksBelow);
    m_targetSlider->setTickInterval(5);
    m_targetValueLabel = new QLabel(QString::number(m_targetSlider->value()), this);
    m_targetValueLabel->setMinimumWidth(30);
    m_targetValueLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    QHBoxLayout* targetLayout = new QHBoxLayout();
    targetLayout->addWidget(m_targetSlider, 1);
    targetLayout->addWidget(m_targetValueLabel);
    formLayout->addRow(targetLabel, targetLayout);
    // 数值实时更新
    connect(m_targetSlider, &QSlider::valueChanged, m_targetValueLabel, [this](int value) {
        m_targetValueLabel->setText(QString::number(value));
        });

    // ========== 新增：4.音频设置区域 ==========
    // 音效开关
    QLabel* audioLabel = new QLabel(tr("settings_audio"), this);
    m_soundEnableCheckBox = new QCheckBox(tr("settings_sound_enable"), this);
    formLayout->addRow(audioLabel, m_soundEnableCheckBox);
    // 音量滑块
    QLabel* volumeLabel = new QLabel(tr("settings_volume"), this);
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setTickPosition(QSlider::TicksBelow);
    m_volumeSlider->setTickInterval(10);
    m_volumeValueLabel = new QLabel("100%", this);
    m_volumeValueLabel->setMinimumWidth(40);
    QHBoxLayout* volumeLayout = new QHBoxLayout();
    volumeLayout->addWidget(m_volumeSlider, 1);
    volumeLayout->addWidget(m_volumeValueLabel);
    formLayout->addRow(volumeLabel, volumeLayout);

    // 【核心联动】音效开关与AudioManager联动
    AppleGameConfig* config = AppleGameConfig::instance();

    connect(m_soundEnableCheckBox, &QCheckBox::toggled, this, [this, config](bool checked) {
        config->setSoundEnabled(checked);
        // 若关闭音效，音量设为0静音；若开启，恢复当前设定的音量
        float vol = checked ? config->getVolume() : 0.0f;
        AudioManager::instance()->setMasterVolume(vol);
        // UI上：关闭音效时禁用音量滑块
        m_volumeSlider->setEnabled(checked);
        });
    // 【核心联动】音量滑块与AudioManager联动
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / 100.0f;
        m_volumeValueLabel->setText(QString::number(value) + "%");
        config->setVolume(val);
        // 只有在音效开启时，滑块才实时调整全局音量
        if (config->isSoundEnabled()) {
            AudioManager::instance()->setMasterVolume(val);
        }
        });

    // 分割线
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #cccccc;");
    formLayout->addRow(line);

    // ========== 新增：后处理参数滑块（直接连接配置单例） ==========

    // 亮度
    QLabel* brightnessLabel = new QLabel(tr("settings_brightness"), this);
    m_brightnessSlider = new QSlider(Qt::Horizontal, this);
    m_brightnessSlider->setRange(-BRIGHTNESS_OFFSET, BRIGHTNESS_OFFSET);
    m_brightnessSlider->setTickPosition(QSlider::TicksBelow);
    m_brightnessSlider->setTickInterval(20);
    m_brightnessValueLabel = new QLabel("0.00", this);
    m_brightnessValueLabel->setMinimumWidth(40);
    QHBoxLayout* brightnessLayout = new QHBoxLayout();
    brightnessLayout->addWidget(m_brightnessSlider, 1);
    brightnessLayout->addWidget(m_brightnessValueLabel);
    formLayout->addRow(brightnessLabel, brightnessLayout);
    connect(m_brightnessSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / BRIGHTNESS_SCALE;
        m_brightnessValueLabel->setText(QString::number(val, 'f', 2));
        config->setBrightness(val); // 直接修改配置，自动触发信号
        });

    // 对比度
    QLabel* contrastLabel = new QLabel(tr("settings_contrast"), this);
    m_contrastSlider = new QSlider(Qt::Horizontal, this);
    m_contrastSlider->setRange(0, 300);
    m_contrastSlider->setTickPosition(QSlider::TicksBelow);
    m_contrastSlider->setTickInterval(50);
    m_contrastValueLabel = new QLabel("1.00", this);
    m_contrastValueLabel->setMinimumWidth(40);
    QHBoxLayout* contrastLayout = new QHBoxLayout();
    contrastLayout->addWidget(m_contrastSlider, 1);
    contrastLayout->addWidget(m_contrastValueLabel);
    formLayout->addRow(contrastLabel, contrastLayout);
    connect(m_contrastSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / CONTRAST_SCALE;
        m_contrastValueLabel->setText(QString::number(val, 'f', 2));
        config->setContrast(val);
        });

    // 饱和度
    QLabel* saturationLabel = new QLabel(tr("settings_saturation"), this);
    m_saturationSlider = new QSlider(Qt::Horizontal, this);
    m_saturationSlider->setRange(0, 300);
    m_saturationSlider->setTickPosition(QSlider::TicksBelow);
    m_saturationSlider->setTickInterval(50);
    m_saturationValueLabel = new QLabel("1.00", this);
    m_saturationValueLabel->setMinimumWidth(40);
    QHBoxLayout* saturationLayout = new QHBoxLayout();
    saturationLayout->addWidget(m_saturationSlider, 1);
    saturationLayout->addWidget(m_saturationValueLabel);
    formLayout->addRow(saturationLabel, saturationLayout);
    connect(m_saturationSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / SATURATION_SCALE;
        m_saturationValueLabel->setText(QString::number(val, 'f', 2));
        config->setSaturation(val);
        });

    // 暗角
    QLabel* vignetteLabel = new QLabel(tr("settings_vignette"), this);
    m_vignetteSlider = new QSlider(Qt::Horizontal, this);
    m_vignetteSlider->setRange(0, 500);
    m_vignetteSlider->setTickPosition(QSlider::TicksBelow);
    m_vignetteSlider->setTickInterval(50);
    m_vignetteValueLabel = new QLabel("0.40", this);
    m_vignetteValueLabel->setMinimumWidth(40);
    QHBoxLayout* vignetteLayout = new QHBoxLayout();
    vignetteLayout->addWidget(m_vignetteSlider, 1);
    vignetteLayout->addWidget(m_vignetteValueLabel);
    formLayout->addRow(vignetteLabel, vignetteLayout);
    connect(m_vignetteSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / VIGNETTE_SCALE;
        m_vignetteValueLabel->setText(QString::number(val, 'f', 2));
        config->setVignette(val);
        });

    // ========== 新增：分割线（区分基础/高级后处理） ==========
    QFrame* advancedLine = new QFrame(this);
    advancedLine->setFrameShape(QFrame::HLine);
    advancedLine->setStyleSheet("background-color: #cccccc;");
    formLayout->addRow(advancedLine);

    // ========== 新增：高级后处理滑块（7个） ==========
   // 1. 高斯模糊
    QLabel* blurLabel = new QLabel(tr("settings_gaussian_blur"), this);
    m_blurRadiusSlider = new QSlider(Qt::Horizontal, this);
    m_blurRadiusSlider->setRange(0, 100);
    m_blurRadiusSlider->setTickPosition(QSlider::TicksBelow);
    m_blurRadiusSlider->setTickInterval(10);
    m_blurRadiusValueLabel = new QLabel("0.00", this);
    m_blurRadiusValueLabel->setMinimumWidth(40);
    QHBoxLayout* blurLayout = new QHBoxLayout();
    blurLayout->addWidget(m_blurRadiusSlider, 1);
    blurLayout->addWidget(m_blurRadiusValueLabel);
    formLayout->addRow(blurLabel, blurLayout);
    connect(m_blurRadiusSlider, &QSlider::valueChanged, this, [this, config](int value) {
    float val = value / BLUR_SCALE;
    m_blurRadiusValueLabel->setText(QString::number(val, 'f', 2));
    config->setBlurRadius(val);
        });

    // 2. 锐化强度
    QLabel* sharpenLabel = new QLabel(tr("settings_sharpen_intensity"), this);
    m_sharpenIntensitySlider = new QSlider(Qt::Horizontal, this);
    m_sharpenIntensitySlider->setRange(0, 200);
    m_sharpenIntensitySlider->setTickPosition(QSlider::TicksBelow);
    m_sharpenIntensitySlider->setTickInterval(20);
    m_sharpenIntensityValueLabel = new QLabel("0.00", this);
    m_sharpenIntensityValueLabel->setMinimumWidth(40);
    QHBoxLayout* sharpenLayout = new QHBoxLayout();
    sharpenLayout->addWidget(m_sharpenIntensitySlider, 1);
    sharpenLayout->addWidget(m_sharpenIntensityValueLabel);
    formLayout->addRow(sharpenLabel, sharpenLayout);
    connect(m_sharpenIntensitySlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / SHARPEN_SCALE;
        m_sharpenIntensityValueLabel->setText(QString::number(val, 'f', 2));
        config->setSharpenIntensity(val);
        });

    // 3. 色调偏移
    QLabel* hueLabel = new QLabel(tr("settings_hue_shift"), this);
    m_hueShiftSlider = new QSlider(Qt::Horizontal, this);
    m_hueShiftSlider->setRange(0, 360);
    m_hueShiftSlider->setTickPosition(QSlider::TicksBelow);
    m_hueShiftSlider->setTickInterval(30);
    m_hueShiftValueLabel = new QLabel(tr("unit_degrees").arg(0), this);
    m_hueShiftValueLabel->setMinimumWidth(40);
    QHBoxLayout* hueLayout = new QHBoxLayout();
    hueLayout->addWidget(m_hueShiftSlider, 1);
    hueLayout->addWidget(m_hueShiftValueLabel);
    formLayout->addRow(hueLabel, hueLayout);
    connect(m_hueShiftSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / HUE_SCALE;
        m_hueShiftValueLabel->setText(tr("unit_degrees").arg(val)); //这里有问题 后续改进
        config->setHueShift(val);
        });

    // 4. 灰度混合
    QLabel* grayscaleLabel = new QLabel(tr("settings_grayscale_mix"), this);
    m_grayscaleMixSlider = new QSlider(Qt::Horizontal, this);
    m_grayscaleMixSlider->setRange(0, 100);
    m_grayscaleMixSlider->setTickPosition(QSlider::TicksBelow);
    m_grayscaleMixSlider->setTickInterval(10);
    m_grayscaleMixValueLabel = new QLabel("0.00", this);
    m_grayscaleMixValueLabel->setMinimumWidth(40);
    QHBoxLayout* grayscaleLayout = new QHBoxLayout();
    grayscaleLayout->addWidget(m_grayscaleMixSlider, 1);
    grayscaleLayout->addWidget(m_grayscaleMixValueLabel);
    formLayout->addRow(grayscaleLabel, grayscaleLayout);
    connect(m_grayscaleMixSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / GRAYSCALE_SCALE;
        m_grayscaleMixValueLabel->setText(QString::number(val, 'f', 2));
        config->setGrayscaleMix(val);
        });

    // 5. 胶片颗粒
    QLabel* grainLabel = new QLabel(tr("settings_film_grain"), this);
    m_filmGrainSlider = new QSlider(Qt::Horizontal, this);
    m_filmGrainSlider->setRange(0, 200);
    m_filmGrainSlider->setTickPosition(QSlider::TicksBelow);
    m_filmGrainSlider->setTickInterval(20);
    m_filmGrainValueLabel = new QLabel("0.000", this);
    m_filmGrainValueLabel->setMinimumWidth(40);
    QHBoxLayout* grainLayout = new QHBoxLayout();
    grainLayout->addWidget(m_filmGrainSlider, 1);
    grainLayout->addWidget(m_filmGrainValueLabel);
    formLayout->addRow(grainLabel, grainLayout);
    connect(m_filmGrainSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / GRAIN_SCALE;
        m_filmGrainValueLabel->setText(QString::number(val, 'f', 3));
        config->setFilmGrain(val);
        });

    // 6. 扫描线强度
    QLabel* scanlineLabel = new QLabel(tr("settings_scanline_intensity"), this);
    m_scanlineIntensitySlider = new QSlider(Qt::Horizontal, this);
    m_scanlineIntensitySlider->setRange(0, 100);
    m_scanlineIntensitySlider->setTickPosition(QSlider::TicksBelow);
    m_scanlineIntensitySlider->setTickInterval(10);
    m_scanlineIntensityValueLabel = new QLabel("0.00", this);
    m_scanlineIntensityValueLabel->setMinimumWidth(40);
    QHBoxLayout* scanlineLayout = new QHBoxLayout();
    scanlineLayout->addWidget(m_scanlineIntensitySlider, 1);
    scanlineLayout->addWidget(m_scanlineIntensityValueLabel);
    formLayout->addRow(scanlineLabel, scanlineLayout);
    connect(m_scanlineIntensitySlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / SCANLINE_SCALE;
        m_scanlineIntensityValueLabel->setText(QString::number(val, 'f', 2));
        config->setScanlineIntensity(val);
        });

    // 7. 像素化大小
    QLabel* pixelationLabel = new QLabel(tr("settings_pixelation_size"), this);
    m_pixelationSizeSlider = new QSlider(Qt::Horizontal, this);
    m_pixelationSizeSlider->setRange(1, 50);
    m_pixelationSizeSlider->setTickPosition(QSlider::TicksBelow);
    m_pixelationSizeSlider->setTickInterval(5);
    m_pixelationSizeValueLabel = new QLabel("1", this);
    m_pixelationSizeValueLabel->setMinimumWidth(40);
    QHBoxLayout* pixelationLayout = new QHBoxLayout();
    pixelationLayout->addWidget(m_pixelationSizeSlider, 1);
    pixelationLayout->addWidget(m_pixelationSizeValueLabel);
    formLayout->addRow(pixelationLabel, pixelationLayout);
    connect(m_pixelationSizeSlider, &QSlider::valueChanged, this, [this, config](int value) {
        float val = value / PIXELATION_SCALE;
        m_pixelationSizeValueLabel->setText(QString::number(val, 'f', 0));
        config->setPixelationSize(val);
        });

    // ========== 原有布局收尾（不变） ==========
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* okBtn = new QPushButton(tr("btn_ok"), this);
    QPushButton* cancelBtn = new QPushButton(tr("btn_cancel"), this);
    okBtn->setFixedWidth(100);
    cancelBtn->setFixedWidth(100);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, this, &AppleGameSettingsDialog::onOkClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &AppleGameSettingsDialog::onCancelClicked);
}

// 加载当前设置（直接从配置单例读取）
void AppleGameSettingsDialog::loadCurrentSettings()
{
    AppleGameConfig* config = AppleGameConfig::instance();

    // 回显分辨率
    QSize res = config->getResolution();
    QString resStr = QString("%1 * %2").arg(res.width()).arg(res.height());
    m_resolutionComboBox->setCurrentText(resStr);

    m_levelSlider->setValue(config->getLevel());
    m_appleCountSlider->setValue(config->getMaxEntityCount());
    m_targetSlider->setValue(config->getPassTarget());

    // ========== 新增：回显音频配置 ==========
    m_soundEnableCheckBox->setChecked(config->isSoundEnabled());
    m_volumeSlider->setValue(qRound(config->getVolume() * 100));
    m_volumeSlider->setEnabled(config->isSoundEnabled());
    m_volumeValueLabel->setText(QString::number(qRound(config->getVolume() * 100)) + "%");

    // 新增：后处理参数
    m_brightnessSlider->setValue(config->getBrightness() * BRIGHTNESS_SCALE);
    m_contrastSlider->setValue(config->getContrast() * CONTRAST_SCALE);
    m_saturationSlider->setValue(config->getSaturation() * SATURATION_SCALE);
    m_vignetteSlider->setValue(config->getVignette() * VIGNETTE_SCALE);

    // ========== 新增：高级后处理参数加载 ==========
    m_blurRadiusSlider->setValue(config->getBlurRadius() * BLUR_SCALE);
    m_sharpenIntensitySlider->setValue(config->getSharpenIntensity() * SHARPEN_SCALE);
    m_hueShiftSlider->setValue(config->getHueShift() * HUE_SCALE);
    m_grayscaleMixSlider->setValue(config->getGrayscaleMix() * GRAYSCALE_SCALE);
    m_filmGrainSlider->setValue(config->getFilmGrain() * GRAIN_SCALE);
    m_scanlineIntensitySlider->setValue(config->getScanlineIntensity() * SCANLINE_SCALE);
    m_pixelationSizeSlider->setValue(config->getPixelationSize() * PIXELATION_SCALE);
}

// 应用设置（仅需处理原有游戏配置，后处理已实时生效）
void AppleGameSettingsDialog::applySettings()
{
    AppleGameConfig* config = AppleGameConfig::instance();

    // 应用分辨率
    QString resStr = m_resolutionComboBox->currentText();
    QStringList parts = resStr.split(" * ");
    if (parts.size() == 2) {
        config->setResolution(QSize(parts[0].toInt(), parts[1].toInt()));
    }

    config->setLevel(m_levelSlider->value());
    config->setMaxEntityCount(m_appleCountSlider->value());
    config->setPassTarget(m_targetSlider->value());
}

void AppleGameSettingsDialog::onOkClicked()
{
    applySettings();
    accept();
}

void AppleGameSettingsDialog::onCancelClicked()
{
    reject();
}