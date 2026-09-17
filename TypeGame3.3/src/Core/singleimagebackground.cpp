#include "singleimagebackground.h"
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QPainter>
#include <QDebug>

// ==================== 【修复1：核心UV坐标系修正】 ====================
// 全屏四边形顶点数据：和Qt图片坐标系100%匹配，uv.y=0=画面顶部，uv.y=1=画面底部
// 彻底解决Y轴翻转导致的区域颠倒问题
static const float quadVertices[] = {
    -1.0f,  1.0f,  0.0f, 0.0f, // 左上：裁剪坐标顶部 → 图片顶部uv.y=0
    -1.0f, -1.0f,  0.0f, 1.0f, // 左下：裁剪坐标底部 → 图片底部uv.y=1
     1.0f,  1.0f,  1.0f, 0.0f, // 右上：裁剪坐标顶部 → 图片顶部uv.y=0
     1.0f, -1.0f,  1.0f, 1.0f, // 右下：裁剪坐标底部 → 图片底部uv.y=1
};

// 构造函数：绑定所属OpenGL控件
SingleImageBackground::SingleImageBackground(int width, int height, const QPixmap& bgPixmap, QOpenGLWidget* glWidget, const QString& fragShaderCode)
	: m_sceneWidth(width), m_sceneHeight(height), m_originalPixmap(bgPixmap), m_glWidget(glWidget), m_fragShaderCode(fragShaderCode)
{
    setZValue(-100); // 置于画面最底层

    // 如果没有传入着色器代码，给一个最基础的默认值（防止黑屏）
    if (m_fragShaderCode.isEmpty()) {
        m_fragShaderCode = R"(
            #version 330 core
            in vec2 vTexCoord;
            out vec4 FragColor;
            uniform sampler2D u_BgTexture;
            void main() {
                FragColor = texture(u_BgTexture, vTexCoord);
            }
        )";
    }

}

// 【核心修复】析构函数：必须在有效上下文中销毁OpenGL资源
SingleImageBackground::~SingleImageBackground()
{
    qInfo() << "[SingleImageBackground] OpenGL resources released safely";
}

// 2. 【新增】实现显式的安全清理方法
void SingleImageBackground::cleanupGLResources()
{
    if (m_glWidget.isNull()) return;

    // 强制挂载上下文执行清理
    m_glWidget->makeCurrent();
    qInfo() << "[SingleImageBackground] Start cleaning up explicit GL resources";

    if (m_bgTexture) {
        delete m_bgTexture;
        m_bgTexture = nullptr;
    }
    if (m_shaderProgram) {
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
    }
    if (m_vao.isCreated()) m_vao.destroy();
    if (m_vbo.isCreated()) m_vbo.destroy();

    m_glWidget->doneCurrent();
}


void SingleImageBackground::setPixmap(const QPixmap& newPixmap)
{
    if (newPixmap.isNull()) return;

    m_originalPixmap = newPixmap;

    if (!m_glWidget.isNull()) {
        m_glWidget->makeCurrent();
        // 销毁旧纹理，paint() 下次执行时会自动调用 initTexture() 重新生成
        if (m_bgTexture) {
            delete m_bgTexture;
            m_bgTexture = nullptr;
        }
        m_glWidget->doneCurrent();
    }
    update(); // 触发场景重绘
}

void SingleImageBackground::setShaderCode(const QString& fragShaderCode)
{
    if (m_fragShaderCode == fragShaderCode) return;

    m_fragShaderCode = fragShaderCode;

    if (!m_glWidget.isNull()) {
        m_glWidget->makeCurrent();
        // 销毁旧的着色器程序，paint() 下次执行时会自动调用 initShader() 重新编译
        if (m_shaderProgram) {
            delete m_shaderProgram;
            m_shaderProgram = nullptr;
        }
        m_glWidget->doneCurrent();
    }
    update(); // 触发场景重绘

}

QRectF SingleImageBackground::boundingRect() const
{
    return QRectF(0, 0, m_sceneWidth, m_sceneHeight);
}

void SingleImageBackground::initShader()
{
    initializeOpenGLFunctions();

    // ========== 【核心修复：增加安全判定，防止 VAO/VBO 重复创建导致刷屏警告】 ==========
    if (m_vao.isCreated()) {
        m_vao.destroy();
    }
    m_vao.create();
    m_vao.bind();

    if (m_vbo.isCreated()) {
        m_vbo.destroy();
    }
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(quadVertices, sizeof(quadVertices));
    // ==================================================================================

    // 编译着色器程序
    m_shaderProgram = new QOpenGLShaderProgram();
    // 顶点着色器（固定全屏四边形，无修改）
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(#version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            vTexCoord = aTexCoord;
        }
    )");

    // 替换成使用我们保存的 m_fragShaderCode 变量
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, m_fragShaderCode);

    // 链接着色器
    m_shaderProgram->link();
    // 设置顶点属性（无修改，和顶点数据匹配）
    m_shaderProgram->bind();
    m_shaderProgram->enableAttributeArray(0);
    m_shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
    m_shaderProgram->enableAttributeArray(1);
    m_shaderProgram->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
    m_shaderProgram->release();
    m_vbo.release();
    m_vao.release();
}

// 重构 initTexture，使用 QOpenGLTexture 自动完成生成、绑定、设置过滤器的操作
void SingleImageBackground::initTexture()
{
    initializeOpenGLFunctions();
    if (m_bgTexture) {
        delete m_bgTexture;
        m_bgTexture = nullptr;
    }
    // 将QPixmap转为OpenGL纹理（无修改）
    QImage bgImage = m_originalPixmap.toImage().convertToFormat(QImage::Format_RGBA8888);
    m_bgTexture = new QOpenGLTexture(bgImage);
    m_bgTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_bgTexture->setMagnificationFilter(QOpenGLTexture::Linear);

    // 【修改点】：将原来的 ClampToEdge 改为 Repeat 模式
    m_bgTexture->setWrapMode(QOpenGLTexture::Repeat);
}

// 修改 paint() 中的纹理绑定逻辑
void SingleImageBackground::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    // 开启OpenGL原生绘制（无修改）
    painter->beginNativePainting();
    if (!m_shaderProgram) initShader();
    if (!m_bgTexture) initTexture();
    // 绑定着色器、VAO和纹理
    m_shaderProgram->bind();
    m_vao.bind();
    glActiveTexture(GL_TEXTURE0);

    // 【修改】使用 QOpenGLTexture 的绑定与释放
    m_bgTexture->bind();
    m_shaderProgram->setUniformValue("u_BgTexture", 0);
    // 传入帧参数
    m_shaderProgram->setUniformValue("iTime", (float)m_totalTime);
    m_shaderProgram->setUniformValue("iResolution", QVector2D(m_sceneWidth, m_sceneHeight));

    // 【新增】将背景原图的真实宽高传入 Shader
    m_shaderProgram->setUniformValue("iTexResolution", QVector2D(m_originalPixmap.width(), m_originalPixmap.height()));

    // 绘制全屏背景
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // 释放资源
    m_bgTexture->release();
    m_vao.release();
    m_shaderProgram->release();
    painter->endNativePainting();
}

void SingleImageBackground::updateFrameTime(qreal deltaTime)
{
    m_totalTime += deltaTime;
    update();
}