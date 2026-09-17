/* ------------------------------------------------------------------
// 文件名     : postprocessglwidget.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : OpenGL 后期处理视口，用于承载特效渲染（已完全解耦）
------------------------------------------------------------------ */
#pragma once
#include <QPointer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QGraphicsView>
#include <QOpenGLPaintDevice>
#include <QMetaObject>

// 【核心修改】前置声明配置基类
class GameConfigBase;

class PostProcessGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit PostProcessGLWidget(QWidget* parent = nullptr);
    ~PostProcessGLWidget();

    PostProcessGLWidget(const PostProcessGLWidget&) = delete;
    PostProcessGLWidget& operator=(const PostProcessGLWidget&) = delete;

    // 绑定游戏视图，内部获取场景用于渲染，避免递归
    void setBindGraphicsView(QGraphicsView* view);

    // 【核心新增】依赖注入：设置通用的配置对象
    void setConfig(GameConfigBase* config);

    QSize sizeHint() const override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
	void keyPressEvent(QKeyEvent* event) override; // 这里不再转发给场景，直接由视图处理，避免递归
    void keyReleaseEvent(QKeyEvent* event) override; 
    void focusOutEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    // 【核心新增】OpenGL资源安全清理槽函数
    void cleanupGLResources();

private:
    void initShader();
    void initFBO(int width, int height);
    void applyPostProcess(GLuint textureId);
    QRectF calcSceneTargetRect() const;
    QPointF mapWidgetPointToScene(const QPointF& widgetPoint) const;

    // 【修改】分别存储视图和场景，避免递归
    QGraphicsView* m_bindView = nullptr;
    QPointer<QGraphicsScene> m_bindScene; // 【核心修复 2】使用 QPointer 自动处理悬空指针
    QMetaObject::Connection m_sceneChangedConnection;

    // 【核心新增】配置对象指针
    GameConfigBase* m_config = nullptr;

    // 双FBO架构
    QOpenGLFramebufferObject* m_renderFbo = nullptr;
    QOpenGLFramebufferObject* m_resolveFbo = nullptr;

    // 后处理着色器资源
    QOpenGLShaderProgram* m_postShader = nullptr;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;

    // 【新增】防递归渲染标志位
    bool m_isRendering = false;

    // 全屏四边形顶点数据
    static const float m_quadVertices[];

};
