/* ------------------------------------------------------------------
// 文件名     : singleton.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 单例模式模板基类，基于 C++11 DCLP 优化，提供高性能的全局访问
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <mutex>
#include <atomic>

/*
  这套方案的优势：
1.零锁开销（Zero Lock Overhead）：当单例初始化完成后，后续所有的 instance() 调用
只会执行一次原子级读取(load)，完全没有系统层面的 Mutex 锁操作。它的执行速度与直
接读取裸指针几乎无异。

2.绝对安全（Thread - Safe）：原生的双重检查可能存在指令重排（Instruction Reordering）的
风险（即指针已分配内存，但对象还没执行完构造函数，另一个线程就拿去用了）。
这里使用了 std::memory_order_acquire 和 std::memory_order_release，强制 CPU 保证
内存分配的时序正确。
*/

// Qt兼容的线程安全单例模板
template<typename T>
class Singleton
{
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    virtual ~Singleton() = default;

    // 全局唯一实例入口
    static T* instance()
    {
        // 第一重检查：如果不为空，直接返回，避开锁的开销（99.99% 的情况均走此逻辑）
        T* tmp = s_instance.load(std::memory_order_acquire);

        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lock(s_mutex);
            // 第二重检查：获取锁后再判断一次，防止多线程同时通过第一重检查
            tmp = s_instance.load(std::memory_order_relaxed);
            if (tmp == nullptr) {
                tmp = new T();
                // 内存屏障：确保 new T() 的内存分配和构造函数执行完毕后，再赋值给指针
                s_instance.store(tmp, std::memory_order_release);
            }
        }
        return tmp;
    }

    // 销毁实例（程序退出时调用）
    static void destroyInstance()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        T* tmp = s_instance.load(std::memory_order_acquire);
        if (tmp != nullptr) {
            delete tmp;
            s_instance.store(nullptr, std::memory_order_release);
        }
    }

protected:
	Singleton() = default; // 构造函数受保护，防止外部直接实例化

private:
    inline static std::atomic<T*> s_instance{ nullptr };
    inline static std::mutex s_mutex;
};