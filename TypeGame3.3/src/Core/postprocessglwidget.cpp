#include "postprocessglwidget.h"
#include "gameconfigbase.h" // 【核心修改】只引入基类配置，不依赖任何具体游戏
#include <QCoreApplication>
#include <QPainter>
#include <QDebug>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>

// 全屏四边形顶点数据 (位置, 纹理坐标)
const float PostProcessGLWidget::m_quadVertices[] = {
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
};

PostProcessGLWidget::PostProcessGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    // 与原游戏保持一致的OpenGL格式
    QSurfaceFormat format;
    format.setSamples(4);
    format.setSwapInterval(1);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    setFormat(format);

    // 关闭自动背景填充，避免Qt默认绘制覆盖后处理结果
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // 初始化防递归标志
    m_isRendering = false;

}

PostProcessGLWidget::~PostProcessGLWidget()
{
    qInfo() << "[PostProcessGLWidget] Destructor start, cleaning up resources";
    cleanupGLResources();
    qInfo() << "[PostProcessGLWidget] Destructor finished";
}

void PostProcessGLWidget::setBindGraphicsView(QGraphicsView* view)
{
    if (m_sceneChangedConnection) {
        disconnect(m_sceneChangedConnection);
    }

    m_bindView = view;
    m_bindScene = nullptr;
    if (view) {
        // 【核心】获取绑定的场景，后续直接渲染场景，避免递归
        m_bindScene = view->scene();
        if (m_bindScene) {
            m_sceneChangedConnection = connect(m_bindScene, &QGraphicsScene::changed, this, [this]() {
                update();
            });
        }
    }
    update();
}

// 【核心新增】配置注入实现
void PostProcessGLWidget::setConfig(GameConfigBase* config)
{
    m_config = config;
    if (m_config) {
        // 如果配置改变，可能需要重新渲染一帧
        connect(m_config, &GameConfigBase::configChanged, this, [this]() {
            update();
            });
    }
    update();
}

QSize PostProcessGLWidget::sizeHint() const
{
    if (m_bindScene) {
        return m_bindScene->sceneRect().size().toSize();
    }
    return QOpenGLWidget::sizeHint();
}

void PostProcessGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    // 全局OpenGL状态初始化
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST); // 2D后处理无需深度测试
    glDisable(GL_STENCIL_TEST);

    initShader();
}

void PostProcessGLWidget::initFBO(int width, int height)
{
    // 释放旧FBO
    if (m_renderFbo) delete m_renderFbo;
    if (m_resolveFbo) delete m_resolveFbo;

    // 1. 多重采样渲染FBO：用于离屏绘制完整游戏场景
    QOpenGLFramebufferObjectFormat renderFormat;
    renderFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    renderFormat.setSamples(4);
    renderFormat.setInternalTextureFormat(GL_RGBA8);
    m_renderFbo = new QOpenGLFramebufferObject(width, height, renderFormat);

    // 2. 单采样解析FBO：用于多重采样内容解析，给着色器采样
    QOpenGLFramebufferObjectFormat resolveFormat;
    resolveFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    resolveFormat.setSamples(0);
    resolveFormat.setInternalTextureFormat(GL_RGBA8);
    m_resolveFbo = new QOpenGLFramebufferObject(width, height, resolveFormat);

    // 打印日志，确认FBO创建成功
    if (!m_renderFbo->isValid()) {
        qDebug() << "Render FBO Fail";
    }
    if (!m_resolveFbo->isValid()) {
        qDebug() << "Analizer FBO Fail";
    }
}

void PostProcessGLWidget::initShader()
{
    m_postShader = new QOpenGLShaderProgram();

    // 顶点着色器
    m_postShader->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            vTexCoord = aTexCoord;
        }
    )");

    // 片段着色器
    m_postShader->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform sampler2D u_SceneTex;
        uniform vec2 u_TextureSize; // 纹理尺寸（用于像素化/模糊采样）

         // 基础效果参数
        uniform float u_Brightness;
        uniform float u_Contrast;
        uniform float u_Saturation;
        uniform float u_Vignette;

        // 新增效果参数
        uniform float u_BlurRadius;    // 高斯模糊半径 [0, 10]
        uniform float u_SharpenIntensity; // 锐化强度 [0, 2]
        uniform float u_HueShift;      // 色调偏移 [0, 360]
        uniform float u_GrayscaleMix;  // 灰度混合 [0=原图, 1=全灰度]
        uniform float u_FilmGrain;     // 胶片颗粒强度 [0, 0.2]
        uniform float u_ScanlineIntensity; // 扫描线强度 [0, 1]
        uniform float u_PixelationSize; // 像素化块大小 [1, 50]

        // 随机噪声（用于胶片颗粒）
        float random(vec2 st) {
            return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
        }
        
         // 高斯模糊采样（3x3 简易版）
        vec3 gaussianBlur(vec2 uv) {
            if (u_BlurRadius <= 0.0) return texture(u_SceneTex, uv).rgb;
            
            vec2 texelSize = 1.0 / u_TextureSize * u_BlurRadius;
            vec3 color = vec3(0.0);
            // 高斯核权重 [0.05, 0.1, 0.05; 0.1, 0.4, 0.1; 0.05, 0.1, 0.05]
            float kernel[9] = float[](
                0.05, 0.1, 0.05,
                0.1,  0.4, 0.1,
                0.05, 0.1, 0.05
            );
            int idx = 0;
            for(int y = -1; y <= 1; y++) {
                for(int x = -1; x <= 1; x++) {
                    vec2 offset = vec2(x, y) * texelSize;
                    color += texture(u_SceneTex, uv + offset).rgb * kernel[idx];
                    idx++;
                }
            }
            return color;
        }

        // 锐化（Unsharp Mask）
        vec3 sharpen(vec3 color, vec2 uv) {
            if (u_SharpenIntensity <= 0.0) return color;
            
            vec2 texelSize = 1.0 / u_TextureSize;
            vec3 blur = texture(u_SceneTex, uv + vec2(-texelSize.x, -texelSize.y)).rgb
                      + texture(u_SceneTex, uv + vec2( texelSize.x, -texelSize.y)).rgb
                      + texture(u_SceneTex, uv + vec2(-texelSize.x,  texelSize.y)).rgb
                      + texture(u_SceneTex, uv + vec2( texelSize.x,  texelSize.y)).rgb;
            blur *= 0.25;
            return mix(color, color + (color - blur) * u_SharpenIntensity, u_SharpenIntensity);
        }

        // 色调偏移
        vec3 hueShift(vec3 color, float hue) {
            hue = radians(hue);
            float cosHue = cos(hue);
            float sinHue = sin(hue);

            // RGB to YIQ转换（适合色调调整）
            vec3 yiq = vec3(
                dot(color, vec3(0.299, 0.587, 0.114)),
                dot(color, vec3(0.596, -0.274, -0.322)),
                dot(color, vec3(0.211, -0.523, 0.312))
            );
            // 色调旋转
            yiq.yz = vec2(
                yiq.y * cosHue - yiq.z * sinHue,
                yiq.y * sinHue + yiq.z * cosHue
            );
            // YIQ back to RGB
            return vec3(
                dot(yiq, vec3(1.0, 0.956, 0.621)),
                dot(yiq, vec3(1.0, -0.272, -0.647)),
                dot(yiq, vec3(1.0, -1.106, 1.703))
            );
        }

        // 像素化
        vec2 pixelate(vec2 uv) {
            if (u_PixelationSize <= 1.0) return uv;
            vec2 pixelSize = u_TextureSize / u_PixelationSize;
            return floor(uv * pixelSize) / pixelSize;
        }

        void main() {
            vec2 uv = vTexCoord;
            // 先做像素化（底层效果）
            uv = pixelate(uv);

            // 1. 高斯模糊
            vec3 color = gaussianBlur(uv);

            // 2. 基础调整：亮度/对比度/饱和度
            color += u_Brightness;
            color = (color - 0.5) * u_Contrast + 0.5;
            float gray = dot(color, vec3(0.299, 0.587, 0.114));
            color = mix(vec3(gray), color, u_Saturation);

            // 3. 锐化（在基础调整后）
            color = sharpen(color, uv);

            // 4. 色调偏移
            color = hueShift(color, u_HueShift);
            
            // 5. 灰度混合
            gray = dot(color, vec3(0.299, 0.587, 0.114));
            color = mix(color, vec3(gray), u_GrayscaleMix);
            
            // 6. 暗角效果
            vec2 vignetteUv = uv - 0.5;
            float vignette = 1.0 - dot(vignetteUv, vignetteUv) * u_Vignette;
            color *= vignette;

            // 7. 胶片颗粒
            if (u_FilmGrain > 0.0) {
                float grain = (random(uv * u_TextureSize) - 0.5) * u_FilmGrain;
                color += grain;
            }

            // 8. 扫描线（模拟老式显示器）
            if (u_ScanlineIntensity > 0.0) {
                float scanline = sin(uv.y * u_TextureSize.y * 2.0) * 0.5 + 0.5;
                scanline = mix(1.0, scanline, u_ScanlineIntensity);
                color *= scanline;
            }

            // 颜色钳制（防止溢出）
            color = clamp(color, 0.0, 1.0);
            FragColor = vec4(color, 1.0);
        }
    )");

    // 链接着色器，检查编译错误
    if (!m_postShader->link()) {
        qDebug() << "Post Process Build Failed:" << m_postShader->log();
        return;
    }

    // 初始化VAO/VBO
    // ========== 【核心修复：增加安全判定，防止 VAO/VBO 重复创建】 ==========
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
    m_vbo.allocate(m_quadVertices, sizeof(m_quadVertices));
    // =========================================================================

    // 设置顶点属性
    m_postShader->enableAttributeArray(0);
    m_postShader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
    m_postShader->enableAttributeArray(1);
    m_postShader->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));

    m_vbo.release();
    m_vao.release();
}

void PostProcessGLWidget::resizeGL(int w, int h)
{
    if (w > 0 && h > 0) {
        initFBO(w, h);
    }
    glViewport(0, 0, w, h);
}

void PostProcessGLWidget::paintGL()
{
    // 【防递归核心】如果正在渲染中，直接返回，避免死循环
    if (m_isRendering || !m_bindScene || !m_renderFbo || !m_resolveFbo) {
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    m_isRendering = true;

    // 步骤 1：离屏渲染游戏场景到多重采样 FBO
    m_renderFbo->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    QOpenGLPaintDevice fboPaintDevice(m_renderFbo->width(), m_renderFbo->height());
    QPainter painter(&fboPaintDevice);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 将场景按比例绘制到FBO中，保证最终显示与输入映射一致
    const QRectF targetRect = calcSceneTargetRect();
    const QRectF sourceRect = m_bindScene->sceneRect();
    m_bindScene->render(&painter, targetRect, sourceRect, Qt::IgnoreAspectRatio);
    painter.end();
    m_renderFbo->release();

    // 步骤 2：多重采样 FBO 解析到单采样 FBO（着色器无法直接采样多重采样纹理）
    QOpenGLFramebufferObject::blitFramebuffer(
        m_resolveFbo, QRect(0, 0, m_resolveFbo->width(), m_resolveFbo->height()),
        m_renderFbo, QRect(0, 0, m_renderFbo->width(), m_renderFbo->height()),
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    // 步骤3：执行后处理，绘制到屏幕
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glClear(GL_COLOR_BUFFER_BIT);
    applyPostProcess(m_resolveFbo->texture());

    // 重置渲染标志位
    m_isRendering = false;
}

void PostProcessGLWidget::applyPostProcess(GLuint textureId)
{
    // paintGL本身处于OpenGL上下文，无需额外包裹
    m_postShader->bind();
    m_vao.bind();

    // 绑定场景纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    m_postShader->setUniformValue("u_SceneTex", 0);

    // ========== 【核心修改】：通过 m_config 获取配置（判空保护） ==========
    if (m_config) {
        m_postShader->setUniformValue("u_Brightness", m_config->getBrightness());
        m_postShader->setUniformValue("u_Contrast", m_config->getContrast());
        m_postShader->setUniformValue("u_Saturation", m_config->getSaturation());
        m_postShader->setUniformValue("u_Vignette", m_config->getVignette());
        m_postShader->setUniformValue("u_BlurRadius", m_config->getBlurRadius());
        m_postShader->setUniformValue("u_SharpenIntensity", m_config->getSharpenIntensity());
        m_postShader->setUniformValue("u_HueShift", m_config->getHueShift());
        m_postShader->setUniformValue("u_GrayscaleMix", m_config->getGrayscaleMix());
        m_postShader->setUniformValue("u_FilmGrain", m_config->getFilmGrain());
        m_postShader->setUniformValue("u_ScanlineIntensity", m_config->getScanlineIntensity());
        m_postShader->setUniformValue("u_PixelationSize", m_config->getPixelationSize());
    }
    else {
        // 万一没有设置配置，提供安全的默认值避免画面全黑
        m_postShader->setUniformValue("u_Brightness", 0.0f);
        m_postShader->setUniformValue("u_Contrast", 1.0f);
        m_postShader->setUniformValue("u_Saturation", 1.0f);
        m_postShader->setUniformValue("u_Vignette", 0.0f);
        m_postShader->setUniformValue("u_BlurRadius", 0.0f);
        m_postShader->setUniformValue("u_SharpenIntensity", 0.0f);
        m_postShader->setUniformValue("u_HueShift", 0.0f);
        m_postShader->setUniformValue("u_GrayscaleMix", 0.0f);
        m_postShader->setUniformValue("u_FilmGrain", 0.0f);
        m_postShader->setUniformValue("u_ScanlineIntensity", 0.0f);
        m_postShader->setUniformValue("u_PixelationSize", 1.0f);
    }

    m_postShader->setUniformValue("u_TextureSize", QVector2D(m_resolveFbo->width(), m_resolveFbo->height()));

    // 绘制全屏四边形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // 释放资源
    glBindTexture(GL_TEXTURE_2D, 0);
    m_vao.release();
    m_postShader->release();

}

QRectF PostProcessGLWidget::calcSceneTargetRect() const
{
    if (!m_bindScene) {
        return QRectF(0, 0, width(), height());
    }

    const QRectF sceneRect = m_bindScene->sceneRect();
    if (sceneRect.isEmpty()) {
        return QRectF(0, 0, width(), height());
    }

    QSizeF targetSize = sceneRect.size();
    targetSize.scale(QSizeF(width(), height()), Qt::KeepAspectRatio);

    const QPointF topLeft(
        (width() - targetSize.width()) * 0.5,
        (height() - targetSize.height()) * 0.5
    );
    return QRectF(topLeft, targetSize);
}

QPointF PostProcessGLWidget::mapWidgetPointToScene(const QPointF& widgetPoint) const
{
    if (!m_bindScene) {
        return QPointF();
    }

    const QRectF targetRect = calcSceneTargetRect();
    const QRectF sceneRect = m_bindScene->sceneRect();
    if (!targetRect.isValid() || !targetRect.contains(widgetPoint) || sceneRect.isEmpty()) {
        return QPointF();
    }

    const qreal xRatio = (widgetPoint.x() - targetRect.left()) / targetRect.width();
    const qreal yRatio = (widgetPoint.y() - targetRect.top()) / targetRect.height();
    return QPointF(
        sceneRect.left() + sceneRect.width() * xRatio,
        sceneRect.top() + sceneRect.height() * yRatio
    );
}

void PostProcessGLWidget::keyPressEvent(QKeyEvent* event)
{
    if (!m_bindScene) {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    QKeyEvent forwardedEvent(
        event->type(),
        event->key(),
        event->modifiers(),
        event->nativeScanCode(),
        event->nativeVirtualKey(),
        event->nativeModifiers(),
        event->text(),
        event->isAutoRepeat(),
        event->count()
    );
    QCoreApplication::sendEvent(m_bindScene, &forwardedEvent);

    if (forwardedEvent.isAccepted()) {
        event->accept();
        return;
    }

    QOpenGLWidget::keyPressEvent(event);
}

void PostProcessGLWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (!m_bindScene) {
        QOpenGLWidget::keyReleaseEvent(event);
        return;
    }

    QKeyEvent forwardedEvent(
        event->type(),
        event->key(),
        event->modifiers(),
        event->nativeScanCode(),
        event->nativeVirtualKey(),
        event->nativeModifiers(),
        event->text(),
        event->isAutoRepeat(),
        event->count()
    );
    QCoreApplication::sendEvent(m_bindScene, &forwardedEvent);

    if (forwardedEvent.isAccepted()) {
        event->accept();
        return;
    }

    QOpenGLWidget::keyReleaseEvent(event);
}

void PostProcessGLWidget::focusOutEvent(QFocusEvent* event)
{
    if (m_bindScene) {
        QFocusEvent forwardedEvent(event->type(), event->reason());
        QCoreApplication::sendEvent(m_bindScene, &forwardedEvent);
    }

    QOpenGLWidget::focusOutEvent(event);
}

void PostProcessGLWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_bindScene) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }

    const QRectF targetRect = calcSceneTargetRect();
    if (!targetRect.contains(event->localPos())) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }

    const QPointF scenePos = mapWidgetPointToScene(event->localPos());
    QGraphicsSceneMouseEvent sceneEvent(QEvent::GraphicsSceneMousePress);
    sceneEvent.setWidget(this);
    sceneEvent.setPos(scenePos);
    sceneEvent.setScenePos(scenePos);
    sceneEvent.setScreenPos(event->globalPos());
    sceneEvent.setButton(event->button());
    sceneEvent.setButtons(event->buttons());
    sceneEvent.setModifiers(event->modifiers());
    sceneEvent.setLastPos(scenePos);
    sceneEvent.setLastScenePos(scenePos);
    sceneEvent.setLastScreenPos(event->globalPos());
    sceneEvent.setButtonDownPos(event->button(), scenePos);
    sceneEvent.setButtonDownScenePos(event->button(), scenePos);
    sceneEvent.setButtonDownScreenPos(event->button(), event->globalPos());
    QCoreApplication::sendEvent(m_bindScene, &sceneEvent);

    if (sceneEvent.isAccepted()) {
        event->accept();
        return;
    }

    QOpenGLWidget::mousePressEvent(event);
}

// 安全清理所有OpenGL资源，此时上下文100%有效
void PostProcessGLWidget::cleanupGLResources()
{
    // ========== 【核心修复：去除 !isValid() 检查】 ==========
    // 在 QOpenGLWidget 的析构周期中，Qt 会保证 makeCurrent() 能拿到清理专用的上下文
    // 直接进行 makeCurrent() 即可，否则会导致真正的内存泄漏

    makeCurrent();
    qInfo() << "[PostProcessGLWidget] Start cleaning up OpenGL resources";

    // 安全销毁所有资源
    if (m_renderFbo) { delete m_renderFbo; m_renderFbo = nullptr; }
    if (m_resolveFbo) { delete m_resolveFbo; m_resolveFbo = nullptr; }
    if (m_postShader) { delete m_postShader; m_postShader = nullptr; }
    if (m_vao.isCreated()) m_vao.destroy();
    if (m_vbo.isCreated()) m_vbo.destroy();

    doneCurrent();
    qInfo() << "[PostProcessGLWidget] OpenGL resources cleaned up successfully";
}
