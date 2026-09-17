#include "backpackdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QToolButton>
#include <QIcon>
#include <QScrollBar>
#include <QPalette>
#include "resourcemanager.h"

namespace
{
    constexpr int ALL_CATEGORY_ID = -1;
    constexpr int CATEGORY_BAR_WIDTH = 86;
    constexpr int CATEGORY_BUTTON_WIDTH = 72;
    constexpr int CATEGORY_BUTTON_HEIGHT = 72;
    constexpr int CATEGORY_ICON_SIZE = 72;

    QString categoryButtonStyleSheet()
    {
        return QStringLiteral(
            "QToolButton#CategoryBtn {"
            "background: transparent;"
            "border: none;"
            "padding: 0px;"
            "margin: 0px;"
            "}"
            "QToolButton#CategoryBtn:hover {"
            "background: rgba(40, 170, 255, 28);"
            "border: 1px solid rgba(90, 220, 255, 90);"
            "border-radius: 6px;"
            "}"
            "QToolButton#CategoryBtn:checked {"
            "background: rgba(40, 170, 255, 40);"
            "border: 1px solid rgba(90, 220, 255, 180);"
            "border-radius: 6px;"
            "}"
        );
    }


    QString itemVerticalScrollBarStyleSheet()
    {
        // 注意：add-page / sub-page 不能设为 transparent。
        // 在部分 Qt 样式或 Windows 环境下，透明会露出系统默认白底，表现为整条白色滚动条。
        return QString::fromUtf8(R"qss(
QScrollBar:vertical {
    width: 14px;
    min-width: 14px;
    max-width: 14px;
    margin: 6px 2px 6px 2px;
    border: none;
    border-radius: 7px;
    background: rgba(1, 8, 22, 230);
}
QScrollBar::groove:vertical {
    width: 14px;
    border: 1px solid rgba(42, 145, 220, 120);
    border-radius: 7px;
    background: rgba(1, 8, 22, 230);
}
QScrollBar::handle:vertical {
    min-height: 42px;
    margin: 2px 2px 2px 2px;
    border: 1px solid rgba(144, 238, 255, 200);
    border-radius: 5px;
    background: qlineargradient(
        spread:pad, x1:0, y1:0, x2:1, y2:0,
        stop:0 rgba(13, 70, 130, 245),
        stop:0.48 rgba(60, 186, 255, 250),
        stop:1 rgba(16, 87, 155, 245)
    );
}
QScrollBar::handle:vertical:hover {
    border: 1px solid rgba(215, 252, 255, 240);
    background: qlineargradient(
        spread:pad, x1:0, y1:0, x2:1, y2:0,
        stop:0 rgba(24, 100, 172, 250),
        stop:0.50 rgba(112, 226, 255, 255),
        stop:1 rgba(24, 100, 172, 250)
    );
}
QScrollBar::handle:vertical:pressed {
    background: rgba(36, 153, 230, 255);
}
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0px;
    width: 0px;
    border: none;
    background: rgba(1, 8, 22, 230);
}
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    border: none;
    border-radius: 6px;
    background: rgba(1, 8, 22, 230);
}
)qss");
    }

    QIcon makeFallbackCategoryIcon(int categoryId, const QString& categoryName, const QSize& size)
    {
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        const QColor iconColor(108, 220, 255, 235);
        const QColor iconHighlight(210, 250, 255, 245);
        painter.setBrush(iconColor);

        if (categoryId == ALL_CATEGORY_ID)
        {
            // “所有”没有资源图时，绘制 3x3 网格图标作为兜底。
            const int cell = qMax(4, size.width() / 5);
            const int gap = qMax(2, size.width() / 14);
            const int total = cell * 3 + gap * 2;
            const int startX = (size.width() - total) / 2;
            const int startY = (size.height() - total) / 2;
            for (int row = 0; row < 3; ++row)
            {
                for (int col = 0; col < 3; ++col)
                {
                    painter.drawRoundedRect(QRectF(startX + col * (cell + gap), startY + row * (cell + gap), cell, cell), 2, 2);
                }
            }
        }
        else
        {
            // 其它分类资源丢失时，用分类名首字兜底，避免按钮空白。
            QFont font = painter.font();
            font.setPixelSize(qMax(18, size.height() / 2));
            font.setBold(true);
            painter.setFont(font);
            painter.setPen(iconHighlight);
            painter.drawText(QRect(QPoint(0, 0), size), Qt::AlignCenter, categoryName.left(1));
        }

        return QIcon(pixmap);
    }
}

static QColor getBackpackQualityColor(ItemQuality quality)
{
    switch (quality)
    {
    case QUALITY_N:         return QColor(168, 172, 178);  // N：灰银
    case QUALITY_R:         return QColor(68, 150, 255);   // R：蓝
    case QUALITY_SR:        return QColor(178, 92, 255);   // SR：紫
    case QUALITY_SSR:       return QColor(255, 205, 68);   // SSR：金
    case QUALITY_SSR_PLUS:  return QColor(255, 116, 46);   // SSR+：橙红
    case QUALITY_UR:        return QColor(108, 246, 255);  // UR：冰蓝幻彩主色，避免和SR紫色混淆
    default:                return QColor(168, 172, 178);
    }
}

BackpackDialog::BackpackDialog(QWidget* parent)
    : QDialog(parent)
{
    // ========== 【最高优先级】窗口核心属性（必须在最前面，所有UI初始化之前） ==========
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setFixedSize(1200, 800);
    setModal(true); // 模态阻塞，避免游戏后台运行
    setObjectName("BackpackDialog"); // 核心：给窗口设置唯一objectName，样式表权重拉满

    // ========== 第二步：初始化数据与UI ==========
    // 初始化测试数据
    BackpackData::instance()->initTestData();
    initUI();

    // ========== 第三步：加载样式表（UI初始化完成后加载） ==========
    QFile backpackStyleFile(":/MainWindow/assets/qss/Backpack_NewStyle.qss");
    if (backpackStyleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&backpackStyleFile);
        stream.setCodec("UTF-8");
        QString styleSheet = stream.readAll();
        this->setStyleSheet(styleSheet);
        backpackStyleFile.close();
    }
    // 分类按钮使用 QToolButton，单独重刷一遍样式，避免外部 qss 中的 QPushButton#CategoryBtn 规则覆盖不到。
    for (auto btn : m_categoryBtnList)
    {
        btn->setStyleSheet(categoryButtonStyleSheet());
    }

    // 最后再给实际的 QScrollBar 设置局部样式；局部样式优先级最高，避免资源 qss 未刷新或父级样式覆盖。
    if (m_itemScrollArea && m_itemScrollArea->verticalScrollBar())
    {
        QScrollBar* verticalBar = m_itemScrollArea->verticalScrollBar();
        verticalBar->setObjectName("ItemVerticalScrollBar");
        verticalBar->setFixedWidth(14);
        verticalBar->setStyleSheet(itemVerticalScrollBarStyleSheet());
        verticalBar->update();
    }

    // ========== 第四步：信号连接与默认逻辑 ==========
    // 连接数据更新信号
    connect(BackpackData::instance(), &BackpackData::dataChanged, this, &BackpackDialog::refreshBackpackData);
    connect(BackpackData::instance(), &BackpackData::resourceChanged, this, &BackpackDialog::refreshResourceInfo);
    // 默认加载“所有”分类
    onCategoryChanged(ALL_CATEGORY_ID);

}

// 【核心工具函数】递归设置所有子控件背景透明，彻底解决白色背景
void BackpackDialog::setAllWidgetTransparent(QWidget* widget)
{
    if (!widget) return;
    // 跳过需要自定义背景的控件
    QStringList ignoreList = { "BodyWidget", "TopBarWidget", "CategoryWidget", "DetailPanel", "CloseBtn" };
    if (!ignoreList.contains(widget->objectName())) {
        widget->setAttribute(Qt::WA_TranslucentBackground, true);
        widget->setAutoFillBackground(false);
    }
    // 递归处理所有子控件
    QList<QWidget*> children = widget->findChildren<QWidget*>();
    for (auto child : children) {
        setAllWidgetTransparent(child);
    }
}

void BackpackDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->setAlignment(Qt::AlignTop);

    // 顶部栏
    initTopBar();
    mainLayout->addWidget(m_topBarWidget);

    // 主体区域（新增objectName，匹配样式表的整体磨砂背景）
    QWidget* bodyWidget = new QWidget(this);
    bodyWidget->setObjectName("BodyWidget"); // 核心：必须设置
    bodyWidget->setAutoFillBackground(true); // 允许样式表设置背景
    QHBoxLayout* bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(20, 10, 20, 20);
    bodyLayout->setSpacing(20);

    // 左侧分类栏
    QWidget* categoryWidget = new QWidget(this);
    categoryWidget->setObjectName("CategoryWidget"); // 核心：必须设置
    categoryWidget->setFixedWidth(CATEGORY_BAR_WIDTH);
    categoryWidget->setAutoFillBackground(true);
    QVBoxLayout* categoryLayout = new QVBoxLayout(categoryWidget);
    categoryLayout->setContentsMargins(0, 0, 0, 0);
    categoryLayout->setSpacing(8);
    initCategoryBar();
    for (auto btn : m_categoryBtnList)
    {
        categoryLayout->addWidget(btn, 0, Qt::AlignHCenter);
    }
    categoryLayout->addStretch();
    bodyLayout->addWidget(categoryWidget);

    // 中间物品网格
    initItemGrid();
    bodyLayout->addWidget(m_itemScrollArea, 1);

    // 右侧详情面板
    initDetailPanel();
    bodyLayout->addWidget(m_detailPanelWidget);

    mainLayout->addWidget(bodyWidget, 1);
}

void BackpackDialog::initTopBar()
{
    m_topBarWidget = new QWidget(this);
    m_topBarWidget->setObjectName("TopBarWidget");
    m_topBarWidget->setFixedHeight(60);
    QHBoxLayout* topLayout = new QHBoxLayout(m_topBarWidget);
    topLayout->setContentsMargins(20, 10, 20, 10);
    topLayout->setSpacing(15);

    // 标题
    QLabel* titleLabel = new QLabel(tr("backpack_title"), this);
    titleLabel->setObjectName("TopBarLabel");
    titleLabel->setStyleSheet("font-size: 20px;");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    // 钻石 铸铁
    QLabel* diamondIcon = new QLabel(this);
    diamondIcon->setFixedSize(24, 24);
    QPixmap diamondPixmap = ResourceManager::instance()->loadPixmap(":/BackPack/assets/images/BackPack/Iron.png");
    if (!diamondPixmap.isNull())
        diamondIcon->setPixmap(diamondPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_diamondLabel = new QLabel(QString::number(BackpackData::instance()->getDiamond()), this);
    m_diamondLabel->setObjectName("TopBarLabel");
    QPushButton* diamondAddBtn = new QPushButton("+", this);
    diamondAddBtn->setFixedSize(24, 24);
    diamondAddBtn->setObjectName("ResourceAddBtn");
    topLayout->addWidget(diamondIcon);
    topLayout->addWidget(m_diamondLabel);
    topLayout->addWidget(diamondAddBtn);

    // 金币 模块
    QLabel* goldIcon = new QLabel(this);
    goldIcon->setFixedSize(24, 24);
    QPixmap goldPixmap = ResourceManager::instance()->loadPixmap(":/BackPack/assets/images/BackPack/mod.png");
    if (!goldPixmap.isNull())
        goldIcon->setPixmap(goldPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_goldLabel = new QLabel(QString::number(BackpackData::instance()->getGold()), this);
    m_goldLabel->setObjectName("TopBarLabel");
    topLayout->addSpacing(20);
    topLayout->addWidget(goldIcon);
    topLayout->addWidget(m_goldLabel);

    // 背包容量
    int total = BackpackData::instance()->getTotalItemCount();
    int max = BackpackData::instance()->getMaxCapacity();
    m_capacityLabel = new QLabel(tr("backpack_capacity").arg(total).arg(max), this);
    m_capacityLabel->setObjectName("TopBarLabel");
    topLayout->addSpacing(30);
    topLayout->addWidget(m_capacityLabel);

    // 关闭按钮
    m_closeBtn = new QPushButton("X", this);
    m_closeBtn->setObjectName("CloseBtn");
    connect(m_closeBtn, &QPushButton::clicked, this, &BackpackDialog::onCloseClicked);
    topLayout->addSpacing(20);
    topLayout->addWidget(m_closeBtn);
}

void BackpackDialog::initCategoryBar()
{
    QList<BackpackCategory> categories = BackpackData::instance()->getAllCategories();
    for (auto& category : categories)
    {
        QToolButton* categoryBtn = new QToolButton(this);
        categoryBtn->setObjectName("CategoryBtn");
        categoryBtn->setCheckable(true);
        categoryBtn->setAutoExclusive(true);
        categoryBtn->setProperty("categoryId", category.categoryId);

        categoryBtn->setFixedSize(CATEGORY_BUTTON_WIDTH, CATEGORY_BUTTON_HEIGHT);

        // 关键：只显示图标，不显示下方文字
        categoryBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        categoryBtn->setText(QString());

        // 如果你连悬停提示也不想要，可以把这一行删掉
        categoryBtn->setToolTip(category.categoryName);

        categoryBtn->setCursor(Qt::PointingHandCursor);
        categoryBtn->setAutoRaise(false);
        categoryBtn->setStyleSheet(categoryButtonStyleSheet());

        QPixmap iconPixmap = ResourceManager::instance()->loadPixmap(category.iconPath);
        if (iconPixmap.isNull())
        {
            iconPixmap.load(category.iconPath);
        }

        if (!iconPixmap.isNull())
        {
            categoryBtn->setIcon(QIcon(iconPixmap));
            categoryBtn->setIconSize(QSize(CATEGORY_ICON_SIZE, CATEGORY_ICON_SIZE));
        }
        else
        {
            categoryBtn->setIcon(makeFallbackCategoryIcon(
                category.categoryId,
                category.categoryName,
                QSize(CATEGORY_ICON_SIZE, CATEGORY_ICON_SIZE)));
            categoryBtn->setIconSize(QSize(CATEGORY_ICON_SIZE, CATEGORY_ICON_SIZE));
        }

        connect(categoryBtn, &QToolButton::clicked, this, [this, category]() {
            onCategoryChanged(category.categoryId);
            });

        m_categoryBtnList.append(categoryBtn);
    }
}

void BackpackDialog::initItemGrid()
{
    m_itemScrollArea = new QScrollArea(this);
    m_itemScrollArea->setObjectName("ItemScrollArea");
    m_itemScrollArea->setWidgetResizable(true);
    m_itemScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_itemScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_itemScrollArea->setFrameShape(QFrame::NoFrame); // 去掉边框
    m_itemScrollArea->setAutoFillBackground(false);

    // 直接作用到实际滚动条，避免只改全局 qss 时被系统默认白色滚动条覆盖。
    QScrollBar* verticalBar = m_itemScrollArea->verticalScrollBar();
    verticalBar->setObjectName("ItemVerticalScrollBar");
    verticalBar->setFixedWidth(14);
    verticalBar->setAutoFillBackground(true);
    QPalette scrollBarPalette = verticalBar->palette();
    scrollBarPalette.setColor(QPalette::Window, QColor(1, 8, 22, 230));
    verticalBar->setPalette(scrollBarPalette);
    verticalBar->setContextMenuPolicy(Qt::NoContextMenu);
    verticalBar->setStyleSheet(itemVerticalScrollBarStyleSheet());

    // 【核心】视口强制透明，彻底解决白色背景
    QWidget* viewport = m_itemScrollArea->viewport();
    viewport->setObjectName("ItemScrollViewport");
    viewport->setAutoFillBackground(false);
    viewport->setAttribute(Qt::WA_TranslucentBackground, true);

    // 物品容器
    QWidget* itemContainer = new QWidget(this);
    itemContainer->setObjectName("ItemContainer");
    itemContainer->setAutoFillBackground(false);
    itemContainer->setAttribute(Qt::WA_TranslucentBackground, true);

    m_itemGridLayout = new QGridLayout(itemContainer);
    m_itemGridLayout->setContentsMargins(10, 10, 10, 10);
    m_itemGridLayout->setSpacing(15);
    m_itemGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_itemScrollArea->setWidget(itemContainer);
}

void BackpackDialog::initDetailPanel()
{
    m_detailPanelWidget = new QWidget(this);
    m_detailPanelWidget->setObjectName("DetailPanel");
    m_detailPanelWidget->setFixedWidth(320);
    m_detailPanelWidget->setAutoFillBackground(true);
    QVBoxLayout* detailLayout = new QVBoxLayout(m_detailPanelWidget);
    detailLayout->setContentsMargins(20, 20, 20, 20);
    detailLayout->setSpacing(15);

    m_itemNameLabel = new QLabel(this);
    m_itemNameLabel->setObjectName("DetailTitleLabel");
    m_itemNameLabel->setWordWrap(true);

    m_itemCategoryLabel = new QLabel(this);
    m_itemCategoryLabel->setObjectName("DetailSubLabel");

    m_itemIconDetailLabel = new QLabel(this);
    m_itemIconDetailLabel->setFixedSize(200, 200);
    m_itemIconDetailLabel->setAlignment(Qt::AlignCenter);

    m_itemDescLabel = new QLabel(this);
    m_itemDescLabel->setObjectName("DetailDescLabel");
    m_itemDescLabel->setWordWrap(true);
    m_itemDescLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // 使用按钮
    m_useBtn = new QPushButton(tr("btn_use_item"), this);
    m_useBtn->setObjectName("UseBtn");
    connect(m_useBtn, &QPushButton::clicked, this, &BackpackDialog::onUseClicked);

    detailLayout->addWidget(m_itemNameLabel);
    detailLayout->addWidget(m_itemCategoryLabel);
    detailLayout->addWidget(m_itemIconDetailLabel, 0, Qt::AlignCenter);
    detailLayout->addSpacing(20);
    detailLayout->addWidget(m_itemDescLabel);
    detailLayout->addStretch();
    detailLayout->addWidget(m_useBtn);

    m_detailPanelWidget->setVisible(false);
}

void BackpackDialog::onCategoryChanged(int categoryId)
{
    m_currentCategoryId = categoryId;
    // 更新分类选中状态
    for (auto btn : m_categoryBtnList)
    {
        btn->setChecked(btn->property("categoryId").toInt() == categoryId);
    }
    refreshBackpackData();
}

void BackpackDialog::onItemClicked(BackpackItemWidget* itemWidget)
{
    // 取消上一个选中
    if (m_currentSelectedItem) m_currentSelectedItem->setSelected(false);
    // 设置新选中
    m_currentSelectedItem = itemWidget;
    m_currentSelectedItem->setSelected(true);

    // 更新详情面板
    BackpackItemData itemData = itemWidget->getItemData();

    QColor qualityColor = getBackpackQualityColor(itemData.quality);

    // 红圈住的整个右侧详情框，根据品质变色；UR 使用冰蓝/金/绿幻彩面板，避免和 SR 紫色相似
    if (itemData.quality == QUALITY_UR)
    {
        m_detailPanelWidget->setStyleSheet(
            "QWidget#DetailPanel {"
            "background: qlineargradient("
            "spread:pad, x1:0, y1:0, x2:1, y2:1,"
            "stop:0 rgba(26, 92, 112, 82),"
            "stop:0.24 rgba(12, 32, 72, 238),"
            "stop:0.46 rgba(80, 66, 24, 88),"
            "stop:0.68 rgba(20, 82, 58, 78),"
            "stop:0.84 rgba(32, 38, 94, 92),"
            "stop:1 rgba(22, 10, 52, 245)"
            ");"
            "border: 1px solid rgba(108, 246, 255, 245);"
            "border-radius: 8px;"
            "}"
        );
    }
    else
    {
        m_detailPanelWidget->setStyleSheet(QString(
            "QWidget#DetailPanel {"
            "background: qlineargradient("
            "spread:pad, x1:0, y1:0, x2:0, y2:1,"
            "stop:0 rgba(%1, %2, %3, 42),"
            "stop:0.35 rgba(8, 30, 60, 238),"
            "stop:1 rgba(2, 9, 24, 245)"
            ");"
            "border: 1px solid rgba(%1, %2, %3, 230);"
            "border-radius: 8px;"
            "}"
        ).arg(qualityColor.red())
            .arg(qualityColor.green())
            .arg(qualityColor.blue()));
    }

    m_itemNameLabel->setText(itemData.itemName);
    m_itemCategoryLabel->setText(itemData.categoryName);
    m_itemDescLabel->setText(itemData.description);

    QPixmap iconPixmap = BackpackData::loadItemIconPixmap(itemData, m_itemIconDetailLabel->size());
    if (!iconPixmap.isNull())
    {
        m_itemIconDetailLabel->setPixmap(iconPixmap);
    }
    m_useBtn->setEnabled(itemData.count > 0);
    m_detailPanelWidget->setVisible(true);
}

void BackpackDialog::onCloseClicked()
{
    accept(); // 关闭对话框，返回Accepted状态
}

void BackpackDialog::refreshBackpackData()
{
    // 清空现有物品
    for (auto item : m_itemWidgetList) item->deleteLater();
    m_itemWidgetList.clear();
    m_currentSelectedItem = nullptr;
    m_detailPanelWidget->setVisible(false);

    // 加载分类物品
    QList<BackpackItemData> itemList = BackpackData::instance()->getItemsByCategory(m_currentCategoryId);
    const int COLUMN_COUNT = 7; // 每行7个，和参考图对齐
    for (int i = 0; i < itemList.size(); ++i)
    {
        int row = i / COLUMN_COUNT;
        int col = i % COLUMN_COUNT;
        BackpackItemWidget* itemWidget = new BackpackItemWidget(itemList[i], this);
        connect(itemWidget, &BackpackItemWidget::itemClicked, this, &BackpackDialog::onItemClicked);
        m_itemGridLayout->addWidget(itemWidget, row, col);
        m_itemWidgetList.append(itemWidget);
    }
    refreshResourceInfo();
}

void BackpackDialog::refreshResourceInfo()
{
    m_diamondLabel->setText(QString::number(BackpackData::instance()->getDiamond()));
    m_goldLabel->setText(QString::number(BackpackData::instance()->getGold()));
    int total = BackpackData::instance()->getTotalItemCount();
    int max = BackpackData::instance()->getMaxCapacity();
    m_capacityLabel->setText(tr("backpack_capacity").arg(total).arg(max));
}

void BackpackDialog::onUseClicked()
{
    // 展示阶段暂不做具体使用功能，保留按钮与选中效果。
    if (!m_currentSelectedItem) return;
}

// 窗口拖动逻辑
void BackpackDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_topBarWidget->underMouse())
    {
        m_isDragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void BackpackDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void BackpackDialog::mouseReleaseEvent(QMouseEvent* event)
{
    m_isDragging = false;
    QDialog::mouseReleaseEvent(event);
}