#include "planegamedata.h"

PlaneGameData::PlaneGameData(PlaneGameConfig* config, QObject* parent)
    : GameDataBase(config, parent)
{
    // 所有的信号连接、状态管理、清理重置逻辑，全部交由 GameDataBase 自动处理。
    // 子类仅做类型强转的桥接，实现真正的面向对象继承复用！
}