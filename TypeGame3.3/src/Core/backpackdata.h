#pragma once
#include <QObject>
#include <QHash>
#include <QList>
#include <QPixmap>
#include <QSize>
#include "singleton.h"
#include "commondefs.h"

// 物品品质枚举（N / R / SR / SSR / SSR+ / UR）
enum ItemQuality
{
    QUALITY_N = 0,          // N级：灰银
    QUALITY_R,              // R级：蓝色
    QUALITY_SR,             // SR级：紫色
    QUALITY_SSR,            // SSR级：金色
    QUALITY_SSR_PLUS,       // SSR+级：橙红
    QUALITY_UR,             // UR级：彩虹/幻彩
};

// 背包物品结构体
struct BackpackItemData
{
    int itemId;                 // 物品唯一ID
    QString itemName;           // 物品名称
    QString iconPath;           // 单张图标资源路径：兼容旧物品
    int count;                  // 物品数量
    ItemQuality quality;        // 物品品质
    QString description;        // 物品描述
    int categoryId;             // 分类ID
    QString categoryName;       // 分类名称

    // 图集切片信息：用于 6x6 等份素材图。iconAtlasPath 为空时使用 iconPath。
    QString iconAtlasPath;
    int iconRow = -1;
    int iconColumn = -1;
    int iconRows = 0;
    int iconColumns = 0;
};

// 背包分类结构体
struct BackpackCategory
{
    int categoryId;
    QString categoryName;
    QString iconPath;
};

class BackpackData : public QObject, public Singleton<BackpackData>
{
    Q_OBJECT
    friend class Singleton<BackpackData>;
public:
    ~BackpackData() override = default;

    // ========= 新增：显式禁用拷贝和赋值 =========
    BackpackData(const BackpackData&) = delete;
    BackpackData& operator=(const BackpackData&) = delete;
    // ===========================================

    // 图标读取：自动判断单图标 / 6x6 图集切片
    static QPixmap loadItemIconPixmap(const BackpackItemData& itemData, const QSize& targetSize = QSize());

    // 分类管理
    QList<BackpackCategory> getAllCategories() const;
    QList<BackpackItemData> getItemsByCategory(int categoryId) const;
    QList<BackpackItemData> getAllItems() const;

    // 物品操作
    void addItem(int itemId, int addCount = 1);
    void reduceItem(int itemId, int reduceCount = 1);

    // 背包容量与资源
    int getTotalItemCount() const;
    int getMaxCapacity() const { return m_maxCapacity; }
    void setMaxCapacity(int capacity) { m_maxCapacity = capacity; }
    int getDiamond() const { return m_diamond; }
    int getGold() const { return m_gold; }
    void setDiamond(int num) { m_diamond = num; }
    void setGold(int num) { m_gold = num; }

    // 测试数据初始化
    void initTestData();

signals:
    void dataChanged(); // 数据变更信号，UI层可以连接此信号以刷新显示
    void resourceChanged(); // 资源变更信号，UI层可以连接此信号以刷新显示

private:
    explicit BackpackData(QObject* parent = nullptr);
    QHash<int, BackpackItemData> m_itemRegistry;  // 物品基础库
    QHash<int, int> m_playerItems;                // 玩家背包物品
    QList<BackpackCategory> m_categories;         // 分类列表
    int m_maxCapacity = 2000;
    int m_diamond = 1;
    int m_gold = 100;
};
