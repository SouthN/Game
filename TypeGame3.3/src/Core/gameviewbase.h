/* ------------------------------------------------------------------
// 文件名     : gameviewbase.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : MVC 视图层基类，负责处理图形渲染、场景管理与动画展现
------------------------------------------------------------------ */
#pragma once
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include "gamedatabase.h"
#include "resourcemanager.h"


// MVC View 基类：负责界面渲染、场景管理、用户输入转发，不包含业务逻辑
class GameViewBase : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GameViewBase(GameDataBase* data, QObject* parent = nullptr)
        : QGraphicsScene(parent), m_gameData(data) {
    }
    ~GameViewBase() override {
    }

    // ========= 新增：禁用拷贝和赋值 =========
    GameViewBase(const GameViewBase&) = delete;
    GameViewBase& operator=(const GameViewBase&) = delete;

    // 纯虚接口：场景初始化与清理
    virtual void initScene(int width, int height) = 0;
    virtual void clearScene() = 0;

    // 【新增】统一的时间轴动画更新接口
    virtual void updateAnimations(float deltaTime) {}

    // 场景尺寸接口，供Controller使用
	int getSceneWidth() const { return m_sceneWidth; }  
    int getSceneHeight() const { return m_sceneHeight; } 

public slots:
    virtual void onDataChanged(int dataType) = 0;
    virtual void onStateChanged(int state) = 0;

signals:
    // 输入事件转发给Controller
    void keyPressed(QKeyEvent* event); 
    void viewClicked(QMouseEvent* event);

protected:
	GameDataBase* m_gameData = nullptr; // 数据模型指针，观察者模式核心
    int m_sceneWidth = 0;
    int m_sceneHeight = 0;

    // 输入事件重写，转发信号
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
};

inline void GameViewBase::keyPressEvent(QKeyEvent* event)
{
    emit keyPressed(event);
    QGraphicsScene::keyPressEvent(event);
}

inline void GameViewBase::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // 手动构造QMouseEvent，转发核心事件信息
    QMouseEvent mouseEvent(QEvent::MouseButtonPress,
        event->screenPos(), 
        event->button(),
        event->buttons(), 
        event->modifiers());
	emit viewClicked(&mouseEvent); // 转发自定义信号，传递核心事件信息
	QGraphicsScene::mousePressEvent(event); // 调用基类处理，保持默认行为
}