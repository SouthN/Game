#include "backpackdata.h"
#include <QRect>
#include <algorithm>
#include "resourcemanager.h"

namespace
{
    constexpr int ITEM_ATLAS_ROWS = 6;
    constexpr int ITEM_ATLAS_COLUMNS = 6;
    const char* ITEM_ATLAS_PATH = ":/BackPack/assets/images/BackPack/Item.png";

    // 辅助函数：创建图集物品数据
    BackpackItemData makeAtlasItem(
        int itemId,
        const QString& itemName,
        int iconRow,
        int iconColumn,
        int count,
        ItemQuality quality,
        const QString& description,
        int categoryId,
        const QString& categoryName)
    {
        return BackpackItemData{
            itemId,
            itemName,
            QString(),       // iconPath：图集物品不使用单图路径
            count,
            quality,
            description,
            categoryId,
            categoryName,
            ITEM_ATLAS_PATH,
            iconRow,
            iconColumn,
            ITEM_ATLAS_ROWS,
            ITEM_ATLAS_COLUMNS
        };
    }

    void sortByItemId(QList<BackpackItemData>& items)
    {
        std::sort(items.begin(), items.end(), [](const BackpackItemData& a, const BackpackItemData& b) {
            return a.itemId < b.itemId;
            });
    }
}

BackpackData::BackpackData(QObject* parent)
    : QObject(parent)
    // 初始化分类（与参考图对应）
    // categoryId = -1 表示“所有”，不参与物品自身分类，只用于界面筛选。
    , m_categories({
        {-1, tr("backpack_category_all"), ":/BackPack/assets/images/BackPack/category_all.png"}, // 所有
        {0, tr("backpack_category_material"), ":/BackPack/assets/images/BackPack/category_material.png"}, // 材料
        {1, tr("backpack_category_weapon"), ":/BackPack/assets/images/BackPack/category_weapon.png"}, // 武器/装备
        {2, tr("backpack_category_item"), ":/BackPack/assets/images/BackPack/category_item.png"}, // 道具
        {3, tr("backpack_category_supply"), ":/BackPack/assets/images/BackPack/category_supply.png"}, // 补给
        {4, tr("backpack_category_quest"), ":/BackPack/assets/images/BackPack/category_quest.png"} // 任务
        })
    // 物品基础库初始化列表：Item.png 为 6行 x 6列图集，row/column 从 0 开始。
    , m_itemRegistry({
        {2001, makeAtlasItem(2001, tr("Heavy Plasma Cannon"), 0, 0, 2, QUALITY_SSR, tr("A high-energy cannon that can be mounted on the front of a hull. Currently used only for backpack display."), 1, tr("backpack_category_weapon"))},
        {2002, makeAtlasItem(2002, tr("Armor-Piercing Star Spear Missile"), 0, 1, 8, QUALITY_SR, tr("A high-speed missile designed to pierce armored hulls. It does not trigger combat logic in the display phase."), 1, tr("backpack_category_weapon"))},
        {2003, makeAtlasItem(2003, tr("Blue-Flame Propulsion Core"), 0, 2, 12, QUALITY_R, tr("A blue energy core for small thrusters, displayed as an upgrade material."), 0, tr("backpack_category_material"))},
        {2004, makeAtlasItem(2004, tr("Quantum Energy Core"), 0, 3, 5, QUALITY_SSR, tr("A core component containing high-density quantum energy, suitable for representing advanced materials."), 0, tr("backpack_category_material"))},
        {2005, makeAtlasItem(2005, tr("Stellar Reactor"), 0, 4, 3, QUALITY_SSR_PLUS, tr("A reactor module with an orange ring-shaped containment field, displayed as a rare item."), 0, tr("backpack_category_material"))},
        {2006, makeAtlasItem(2006, tr("Pulse Crystal Core"), 0, 5, 1, QUALITY_UR, tr("An ultimate energy core with continuous blue pulses, used to highlight the highest quality tier."), 0, tr("backpack_category_material"))},

        {2007, makeAtlasItem(2007, tr("Amethyst Control Board"), 1, 0, 4, QUALITY_SR, tr("A control board etched with purple circuits, displayed as a story-unlock item."), 4, tr("backpack_category_quest"))},
        {2008, makeAtlasItem(2008, tr("Blue-Ring Scope"), 1, 1, 6, QUALITY_R, tr("An auxiliary aiming module with a blue reticle ring."), 1, tr("backpack_category_weapon"))},
        {2009, makeAtlasItem(2009, tr("Portable Energy Tower"), 1, 2, 7, QUALITY_R, tr("A temporarily deployable energy-gathering device, currently displayed as a common item."), 2, tr("backpack_category_item"))},
        {2010, makeAtlasItem(2010, tr("Defensive Base Module"), 1, 3, 3, QUALITY_SR, tr("A stabilizing base for heavy equipment, with a blue energy pillar."), 2, tr("backpack_category_item"))},
        {2011, makeAtlasItem(2011, tr("Emerald Energy Canister"), 1, 4, 15, QUALITY_R, tr("A restorative green energy supply canister, used only for supply item display."), 3, tr("backpack_category_supply"))},
        {2012, makeAtlasItem(2012, tr("Amethyst Energy Canister"), 1, 5, 9, QUALITY_SR, tr("An advanced energy supply canister containing amethyst particles."), 3, tr("backpack_category_supply"))},

        {2013, makeAtlasItem(2013, tr("Amethyst Cluster"), 2, 0, 24, QUALITY_SR, tr("A raw mineral material composed of multiple purple crystals."), 0, tr("backpack_category_material"))},
        {2014, makeAtlasItem(2014, tr("Titanium Alloy Ingot"), 2, 1, 36, QUALITY_N, tr("A common metal ingot, displayed as a basic crafting material."), 0, tr("backpack_category_material"))},
        {2015, makeAtlasItem(2015, tr("Blue Supply Crate"), 2, 2, 10, QUALITY_R, tr("A standard blue supply crate, suitable for common loot display."), 2, tr("backpack_category_item"))},
        {2016, makeAtlasItem(2016, tr("Golden Equipment Crate"), 2, 3, 4, QUALITY_SSR, tr("An advanced equipment crate with a golden latch, used for rare drop display."), 2, tr("backpack_category_item"))},
        {2017, makeAtlasItem(2017, tr("Green Upgrade Crate"), 2, 4, 6, QUALITY_SR, tr("A green supply crate used to store upgrade parts."), 2, tr("backpack_category_item"))},
        {2018, makeAtlasItem(2018, tr("Medical First-Aid Kit"), 2, 5, 12, QUALITY_R, tr("An emergency supply kit with a red medical emblem."), 3, tr("backpack_category_supply"))},

        {2019, makeAtlasItem(2019, tr("Blue Energy Battery"), 3, 0, 20, QUALITY_R, tr("A basic blue battery, displayed as a general consumable item."), 2, tr("backpack_category_item"))},
        {2020, makeAtlasItem(2020, tr("Overload Fuel Canister"), 3, 1, 9, QUALITY_SR, tr("A red high-pressure fuel canister. Dangerous, but powerful."), 1, tr("backpack_category_weapon"))},
        {2021, makeAtlasItem(2021, tr("Precision Observation Lens"), 3, 2, 5, QUALITY_R, tr("A lens component used to calibrate weapons and scanning devices."), 0, tr("backpack_category_material"))},
        {2022, makeAtlasItem(2022, tr("Encrypted Amethyst Chip"), 3, 3, 3, QUALITY_SR, tr("A purple chip written with encrypted commands, usable as quest proof."), 4, tr("backpack_category_quest"))},
        {2023, makeAtlasItem(2023, tr("Triangular Star Core"), 3, 4, 2, QUALITY_SSR_PLUS, tr("A blue star core enclosed in a triangular frame, visually suited for story items."), 4, tr("backpack_category_quest"))},
        {2024, makeAtlasItem(2024, tr("Spherical Navigation Core"), 3, 5, 2, QUALITY_SSR, tr("A navigation array embedded in a spherical mechanical shell, displayed as a quest collectible."), 4, tr("backpack_category_quest"))},

        {2025, makeAtlasItem(2025, tr("Scout Spider"), 4, 0, 4, QUALITY_SR, tr("A four-legged scouting machine, displayed as a tactical unit icon."), 1, tr("backpack_category_weapon"))},
        {2026, makeAtlasItem(2026, tr("Escort Drone"), 4, 1, 5, QUALITY_R, tr("A small escort drone, suitable for standard equipment display."), 1, tr("backpack_category_weapon"))},
        {2027, makeAtlasItem(2027, tr("Gravity Mine"), 4, 2, 7, QUALITY_SSR, tr("A spherical mine surrounded by red nodes, displayed as a high-risk weapon."), 1, tr("backpack_category_weapon"))},
        {2028, makeAtlasItem(2028, tr("Disc Shield Generator"), 4, 3, 2, QUALITY_SSR, tr("A disc-shaped shield device, displayed as an advanced item."), 2, tr("backpack_category_item"))},
        {2029, makeAtlasItem(2029, tr("Mechanical Repair Arm"), 4, 4, 3, QUALITY_R, tr("A repair arm that deploys quickly, displayed as a support item."), 2, tr("backpack_category_item"))},
        {2030, makeAtlasItem(2030, tr("Binocular Long-Range Detector"), 4, 5, 6, QUALITY_R, tr("A binocular long-range detection device."), 2, tr("backpack_category_item"))},

        {2031, makeAtlasItem(2031, tr("Warp Ring Hub"), 5, 0, 1, QUALITY_UR, tr("A warp hub set within a red-and-white metal ring, suitable as a finale quest item."), 4, tr("backpack_category_quest"))},
        {2032, makeAtlasItem(2032, tr("Micro Shuttle"), 5, 1, 1, QUALITY_SR, tr("A small shuttle with blue exhaust, displayed as special equipment."), 1, tr("backpack_category_weapon"))},
        {2033, makeAtlasItem(2033, tr("Twin Turbo Cannon"), 5, 2, 2, QUALITY_SSR_PLUS, tr("Heavy equipment combining twin barrels with a turbine core."), 1, tr("backpack_category_weapon"))},
        {2034, makeAtlasItem(2034, tr("Amethyst Mystery Box"), 5, 3, 2, QUALITY_SSR, tr("A mysterious box sealed with purple energy. It is only displayed for now and cannot be opened."), 4, tr("backpack_category_quest"))},
        {2035, makeAtlasItem(2035, tr("Golden Mystery Box"), 5, 4, 1, QUALITY_SSR_PLUS, tr("An advanced mystery box sealed by a golden energy lock."), 4, tr("backpack_category_quest"))},
        {2036, makeAtlasItem(2036, tr("Blue Star Core"), 5, 5, 1, QUALITY_UR, tr("Blue starlight gathers inside a spherical frame, displayed as an ultimate quest reward."), 4, tr("backpack_category_quest"))}

        })
{
}

QPixmap BackpackData::loadItemIconPixmap(const BackpackItemData& itemData, const QSize& targetSize)
{
    QPixmap sourcePixmap;

    if (!itemData.iconAtlasPath.isEmpty()
        && itemData.iconRows > 0
        && itemData.iconColumns > 0
        && itemData.iconRow >= 0
        && itemData.iconColumn >= 0
        && itemData.iconRow < itemData.iconRows
        && itemData.iconColumn < itemData.iconColumns)
    {
        sourcePixmap = ResourceManager::instance()->loadPixmap(itemData.iconAtlasPath);
        if (sourcePixmap.isNull()) sourcePixmap.load(itemData.iconAtlasPath);
        if (!sourcePixmap.isNull())
        {
            const int cellWidth = sourcePixmap.width() / itemData.iconColumns;
            const int cellHeight = sourcePixmap.height() / itemData.iconRows;
            const int x = itemData.iconColumn * cellWidth;
            const int y = itemData.iconRow * cellHeight;
            sourcePixmap = sourcePixmap.copy(QRect(x, y, cellWidth, cellHeight));
        }
    }
    else if (!itemData.iconPath.isEmpty())
    {
        sourcePixmap = ResourceManager::instance()->loadPixmap(itemData.iconPath);
        if (sourcePixmap.isNull()) sourcePixmap.load(itemData.iconPath);
    }

    if (!sourcePixmap.isNull() && targetSize.isValid())
    {
        sourcePixmap = sourcePixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return sourcePixmap;
}

QList<BackpackCategory> BackpackData::getAllCategories() const
{
    return m_categories;
}

QList<BackpackItemData> BackpackData::getItemsByCategory(int categoryId) const
{
    // “所有”分类使用 -1，只用于筛选显示：直接返回全部玩家拥有的物品。
    if (categoryId < 0)
    {
        return getAllItems();
    }

    QList<BackpackItemData> result;
    for (auto itemId : m_playerItems.keys())
    {
        if (!m_itemRegistry.contains(itemId)) continue;
        BackpackItemData item = m_itemRegistry[itemId];
        if (item.categoryId == categoryId && m_playerItems[itemId] > 0)
        {
            item.count = m_playerItems[itemId];
            result.append(item);
        }
    }
    sortByItemId(result);
    return result;
}

QList<BackpackItemData> BackpackData::getAllItems() const
{
    QList<BackpackItemData> result;
    for (auto itemId : m_playerItems.keys())
    {
        if (!m_itemRegistry.contains(itemId) || m_playerItems[itemId] <= 0) continue;
        BackpackItemData item = m_itemRegistry[itemId];
        item.count = m_playerItems[itemId];
        result.append(item);
    }
    sortByItemId(result);
    return result;
}

void BackpackData::addItem(int itemId, int addCount)
{
    if (!m_itemRegistry.contains(itemId) || addCount <= 0) return;
    m_playerItems[itemId] = m_playerItems.value(itemId, 0) + addCount;
    emit dataChanged();
}

void BackpackData::reduceItem(int itemId, int reduceCount)
{
    if (!m_playerItems.contains(itemId) || reduceCount <= 0) return;
    int newCount = m_playerItems[itemId] - reduceCount;
    newCount <= 0 ? m_playerItems.remove(itemId) : m_playerItems[itemId] = newCount;
    emit dataChanged();  // 物品数量变化后通知界面更新
}

int BackpackData::getTotalItemCount() const
{
    int total = 0;
    for (auto count : m_playerItems.values()) total += count;
    return total;
}

void BackpackData::initTestData()
{
    for (auto itemId : m_itemRegistry.keys())
    {
        m_playerItems[itemId] = m_itemRegistry[itemId].count;
    }
    emit dataChanged();
}
