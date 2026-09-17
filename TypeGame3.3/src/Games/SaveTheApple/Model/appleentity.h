/* ------------------------------------------------------------------
// 文件名     : appleentity.h
// 功能描述   : 苹果实体类，继承自 GameEntity，只保留特有的渲染与销毁逻辑
------------------------------------------------------------------ */
#pragma once
#include "gameentity.h"
#include "applegameconfig.h"

class AppleEntity : public GameEntity // 【修改】只能继承 GameEntity，不能再继承 QGraphicsObject
{
    Q_OBJECT

public:
    static constexpr int APPLE_WIDTH = 60;
    static constexpr int APPLE_HEIGHT = 60;

    AppleEntity();
    ~AppleEntity() override = default;

    AppleEntity(const AppleEntity&) = delete;
    AppleEntity& operator=(const AppleEntity&) = delete;

    // 只需要重写特有的渲染和销毁接口，其余的下落、动画、选中判定全交给基类！
    void paint(QPainter* painter) override;
    void destroy() override;
};