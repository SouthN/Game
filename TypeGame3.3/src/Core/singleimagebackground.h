/* ------------------------------------------------------------------
// 文件名     : singleimagebackground.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : OpenGL 单张背景图特效渲染（预留扩展）
------------------------------------------------------------------ */
#pragma once
#include <QOpenGLTexture>
#include <QGraphicsItem>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QPixmap>
#include <QOpenGLWidget>
#include <QPointer> // 新增：安全指针，避免悬空指针


class SingleImageBackground : public QGraphicsItem, protected QOpenGLFunctions
{
public:
    // 构造函数新增OpenGLWidget参数，绑定所属上下文
    // 【修改点 1】：在参数列表最后新增 const QString& fragShaderCode = ""
    SingleImageBackground(int width, int height, const QPixmap& bgPixmap, QOpenGLWidget* glWidget = nullptr, const QString& fragShaderCode = "");
    ~SingleImageBackground() override;

    // 公开的显式资源清理接口
    void cleanupGLResources();

    // 【核心新增】运行时动态修改背景贴图接口
    void setPixmap(const QPixmap& newPixmap);

    // 【核心新增】运行时动态修改片段着色器接口
    void setShaderCode(const QString& fragShaderCode);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void updateFrameTime(qreal deltaTime); // 每帧更新时间，由游戏循环调用
    
    // 新增：动态设置所属OpenGL上下文控件
    void setGLWidget(QOpenGLWidget* glWidget) { m_glWidget = glWidget; }

private:
	void initShader();  // 初始化着色器程序
	void initTexture();  // 初始化纹理资源

    // 【修改点 2】：新增一个变量保存传入的着色器代码
    QString m_fragShaderCode;

    int m_sceneWidth;
    int m_sceneHeight;
    QPixmap m_originalPixmap;
    QOpenGLTexture* m_bgTexture = nullptr; // 【修改】使用 Qt 的智能纹理类替代 GLuint 裸指针
    qreal m_totalTime = 0.0;
    // 新增：所属OpenGL控件，用QPointer自动处理悬空指针
    QPointer<QOpenGLWidget> m_glWidget;

    // OpenGL资源
    QOpenGLShaderProgram* m_shaderProgram = nullptr;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
};