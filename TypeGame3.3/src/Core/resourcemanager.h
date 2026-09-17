/* ------------------------------------------------------------------
// 文件名     : resourcemanager.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 图片资源管理器，集中处理 QPixmap 的加载与缓存优化
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QPixmap>
#include <QHash>
#include <QString>
#include "singleton.h"

class ResourceManager : public QObject, public Singleton<ResourceManager>
{
    Q_OBJECT
    friend class Singleton<ResourceManager>;
public:
    ~ResourceManager() override = default;

    // ========= 新增：显式禁用拷贝和赋值 =========
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    // ===========================================

    QPixmap loadPixmap(const QString& filePath);
    QPixmap getPixmap(const QString& filePath) const;
    void releasePixmap(const QString& filePath);
    void releaseAll();
    bool isPixmapCached(const QString& filePath) const;

private:
    explicit ResourceManager(QObject* parent = nullptr);
	QHash<QString, QPixmap> m_pixmapCache; // 图片资源缓存，Key为文件路径，Value为加载的QPixmap对象
};