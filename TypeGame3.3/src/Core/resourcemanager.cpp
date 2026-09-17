#include "resourcemanager.h"
#include <QDir>
#include <QDebug>

ResourceManager::ResourceManager(QObject* parent) : QObject(parent) {}

QPixmap ResourceManager::loadPixmap(const QString& filePath)
{
	if (m_pixmapCache.contains(filePath)) return m_pixmapCache[filePath]; // 如果缓存中已经存在对应的图片，直接返回缓存中的 QPixmap 对象
    QPixmap pixmap;
	QString absolutePath = QDir::cleanPath(filePath); // 规范化路径，去除冗余的 "./" 和 "../" 等部分，确保路径正确
    if (!pixmap.load(absolutePath)) {
        qWarning() << "加载图片失败: " << absolutePath;
        return QPixmap();
    }
    m_pixmapCache.insert(filePath, pixmap);
    return pixmap;
}

QPixmap ResourceManager::getPixmap(const QString& filePath) const
{
	return m_pixmapCache.value(filePath, QPixmap()); // 如果缓存中没有对应的图片，返回一个空的 QPixmap 对象
}

void ResourceManager::releasePixmap(const QString& filePath)
{
    if (m_pixmapCache.contains(filePath)) m_pixmapCache.remove(filePath);
}

void ResourceManager::releaseAll()
{
    m_pixmapCache.clear();
}

bool ResourceManager::isPixmapCached(const QString& filePath) const
{
    return m_pixmapCache.contains(filePath);
}