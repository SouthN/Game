#include "applegamedata.h"

AppleGameData::AppleGameData(AppleGameConfig* config, QObject* parent)
    : GameDataBase(config, parent) 
{
    // 所有的信号连接（如统计数据更新、成功/失败累加等）
    // 均已通过 GameDataBase::initBaseConnection() 在基类自动完成！

    // resetData 和 内存释放析构逻辑
    // 均已通过基类多态 entity->destroy() 自动完成！0代码冗余！
}



