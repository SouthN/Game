#pragma once
#include <cstdlib>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <algorithm>
#include <vector> // 【新增】引入 vector 管理内存块

// 非模板基类，用于通过基类指针安全地删除内存池实例
class MemoryPoolBase
{
public:
    virtual ~MemoryPoolBase() = default;
};

// 通用固定大小内存池模板，适配游戏实体生命周期管理
template<typename T>
class MemoryPool : public MemoryPoolBase
{
public:
    explicit MemoryPool(size_t blockCount, bool autoExpand = true)
        : m_blockCount(0), m_autoExpand(autoExpand), m_freeList(nullptr)
    {
        m_blockSize = std::max(sizeof(T), sizeof(FreeNode*));
        expandPool(blockCount); // 初始化第一块内存
    }

    ~MemoryPool() override {
        // 【核心修复】：遍历释放所有独立的内存块，不再使用 realloc 导致地址变更
        for (char* chunk : m_chunks) {
            std::free(chunk);
        }
    }

    // 分配内存并构造对象
    T* allocate()
    {
        if (m_freeList == nullptr) {
            if (!m_autoExpand) return nullptr;
            expandPool(100); // 每次扩容100个单位
        }
        FreeNode* node = m_freeList;
        m_freeList = m_freeList->next;
        return new (static_cast<void*>(node)) T();
    }

    // 析构对象并回收内存到池
    void deallocate(T* ptr)
    {
        if (ptr == nullptr) return;
        ptr->~T();
        FreeNode* node = static_cast<FreeNode*>(static_cast<void*>(ptr));
        node->next = m_freeList;
        m_freeList = node;
    }

private:
    union FreeNode {
        FreeNode* next;
        char data[sizeof(T)]; // 占位符，确保节点大小足够存储 T 对象
    };

    size_t m_blockCount;
    size_t m_blockSize;
    std::vector<char*> m_chunks; // 【核心修复】存储所有分配的内存块指针
    FreeNode* m_freeList;
    bool m_autoExpand;

    void expandPool(size_t expandCount)
    {
        // 【核心修复】：分配全新的独立内存块，绝对不移动老数据！
        char* newChunk = static_cast<char*>(std::malloc(m_blockSize * expandCount));
        if (newChunk == nullptr) throw std::bad_alloc();

        m_chunks.push_back(newChunk); // 记录这块新内存以便最终释放

        // 将新分配的块串入空闲链表
        for (size_t i = 0; i < expandCount; ++i) {
            char* blockAddr = newChunk + i * m_blockSize;
            FreeNode* node = static_cast<FreeNode*>(static_cast<void*>(blockAddr));
            node->next = m_freeList;
            m_freeList = node;
        }
        m_blockCount += expandCount;
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
};