#pragma once

#include <QObject>
#include <typeindex>
#include <QHashFunctions> // 必须先引入基础的 qHash 声明
// ========== 【核心修复：解决 MSVC C2665 找不到 qHash 的问题】 ==========
// 关键点：由于 std::type_index 属于 std 命名空间，
// Qt 在实例化 QHash 模板时，会根据 C++ 的 ADL 规则去 std 命名空间下寻找。
namespace std {
    inline uint qHash(const std::type_index& key, uint seed = 0) {
        // 使用 unsigned long long 兼容 32 位和 64 位环境下的 hash_code 返回值
        return ::qHash(static_cast<unsigned long long>(key.hash_code()), seed);
    }
}
// ==================================================================

#include <QHash>
#include <QGraphicsItem>
#include <QGraphicsScene> 

#include "singleton.h"
#include "memorypool.h"
#include "gameentity.h"

// ========== 【核心修复 1：为 QHash 提供 std::type_index 的哈希函数】 ==========
// Qt 默认没有提供对 std::type_index 的 qHash 支持，需要手动补充
// 这样即使 hash_code 碰撞，QHash 也会回退到安全的 operator== 进行类型区分
inline uint qHash(const std::type_index& key, uint seed = 0) {
    return qHash(static_cast<qulonglong>(key.hash_code()), seed);
}
// ===========================================================================

// 实体工厂：统一管理所有游戏实体的创建与销毁，结合内存池实现高性能内存管理
class EntityFactory : public QObject, public Singleton<EntityFactory>
{
    Q_OBJECT
        friend class Singleton<EntityFactory>;
public:
    ~EntityFactory() override = default;

    // ========= 新增：显式禁用拷贝和赋值 =========
    EntityFactory(const EntityFactory&) = delete;
    EntityFactory& operator=(const EntityFactory&) = delete;
    // ===========================================

    // 创建实体：从对应类型的内存池分配
    template<typename T>
    T* createEntity()
    {
        static_assert(std::is_base_of<GameEntity, T>::value, "T must inherit from GameEntity");

        // ========== 【核心修复 2：使用安全的 type_index】 ==========
        std::type_index typeId(typeid(T));

        // 不存在对应内存池则创建（默认初始块数100，自动扩容）
        if (!m_entityPools.contains(typeId)) {
            m_entityPools[typeId] = new MemoryPool<T>(100, true);
        }
        MemoryPool<T>* pool = static_cast<MemoryPool<T>*>(m_entityPools[typeId]);
        T* entity = pool->allocate();
		entity->setAlive(true); // 标记实体为活跃状态，确保在游戏循环中被更新和渲染
        return entity;
    }

    // 销毁实体：回收内存到对应内存池
    template<typename T>
    void destroyEntity(T* entity)
    {
        static_assert(std::is_base_of<GameEntity, T>::value, "T must inherit from GameEntity");
        if (!entity) return;

        // ========== 【核心修复：内存池扩展性隐患】 ==========
        // 在内存被内存池接管复用之前，必须将其从 Qt 的图形场景中强制剥离。
        // 防止 Qt 底层 BSP 渲染树残留野指针导致崩溃。
        QGraphicsItem* graphicsItem = dynamic_cast<QGraphicsItem*>(entity);
        if (graphicsItem && graphicsItem->scene()) {
            graphicsItem->scene()->removeItem(graphicsItem);
        }
        // ====================================================

        // ========== 【核心修复 3：使用安全的 type_index】 ==========
        std::type_index typeId(typeid(T));

        if (!m_entityPools.contains(typeId)) {
            delete entity;
            return;
        }
		entity->setAlive(false); // 标记实体为不活跃，等待回收
        MemoryPool<T>* pool = static_cast<MemoryPool<T>*>(m_entityPools[typeId]);
        pool->deallocate(entity);
    }

    // 释放所有类型的内存池（程序退出时调用）
    void releaseAllPools()
    {
        for (auto pool : m_entityPools) {
            delete pool;
        }
        m_entityPools.clear();
    }

private:
    explicit EntityFactory(QObject* parent = nullptr) : QObject(parent) {}

    // ========== 【核心修复 4：替换 QHash 的 Key 类型】 ==========
    QHash<std::type_index, MemoryPoolBase*> m_entityPools;
};