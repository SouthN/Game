/* ------------------------------------------------------------------
// 文件名     : applegameview.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果视图层，负责渲染苹果、篮子及游戏内特效动画
------------------------------------------------------------------ */
#pragma once
#include "gameviewbase.h"
#include "applegamedata.h"
#include "singleimagebackground.h" // 新增：单图背景类
#include <QOpenGLWidget> // 新增

// 拯救苹果游戏视图（MVC-View）
class AppleGameView : public GameViewBase
{
    Q_OBJECT
public:
    explicit AppleGameView(AppleGameData* data, QObject* parent = nullptr);
    ~AppleGameView() override;

    // 新增：动态更新分辨率，不销毁场景现有实体
    void updateResolution(int width, int height);

    // 场景接口实现
    void initScene(int width, int height) override;
    void clearScene() override;

    // 苹果渲染管理
    void addAppleToScene(AppleEntity* apple);
	void removeAppleFromScene(AppleEntity* apple); // 从场景中移除苹果图元，但不删除对象，交由Controller统一管理生命周期
    void updateAllApples();

    // 成功计数小苹果管理
    void spawnSuccessAppleIcon();
    void clearSuccessAppleIcons();

    // 新增 OpenGL 控件绑定接口
    void setGLWidget(QOpenGLWidget* glWidget) { m_glWidget = glWidget; }

public slots:
    void onDataChanged(int dataType) override;
    void onStateChanged(int state) override;

signals:
    // 新增：弹窗按钮点击信号
    void nextLevelClicked();
    void restartGameClicked();
    void exitGameClicked();

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override; 
    // 【新增】前景层绘制：用于渲染顶层弹窗，不会被Item覆盖
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    // 新增：重写鼠标事件处理弹窗点击
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    AppleGameData* getGameData() { return static_cast<AppleGameData*>(m_gameData); }
    void initBasket();

	QGraphicsPixmapItem* m_basketItem = nullptr; // 篮子图元
	QList<QGraphicsPixmapItem*> m_successAppleIcons; // 成功计数小苹果图标列表，按照生成顺序排列，方便定位和清理

    // 计数小苹果配置
    static constexpr int SMALL_APPLE_WIDTH = 30;
    static constexpr int SMALL_APPLE_HEIGHT = 30;
    static constexpr int SMALL_APPLE_SPACING = 5;
    void updateBasketAndIconsLayout(); // 新增：统一管理篮子和图标的动态排版


    // 新增：弹窗绘制方法
	void drawLevelCompleteDialog(QPainter* painter); // 绘制关卡完成弹窗
	void drawGameOverDialog(QPainter* painter); // 绘制游戏结束弹窗

    // 新增：按钮区域（用于点击检测）
    QRectF m_nextLevelBtnRect;
    QRectF m_restartBtnRect;
    QRectF m_exitBtnRect;

    // ============== 渲染效果 ================ //
	SingleImageBackground* m_shaderBackground = nullptr; // 新增：使用单图背景类来实现着色器效果，简化渲染逻辑
    QOpenGLWidget* m_glWidget = nullptr;  // 新增：所属OpenGL控件

private slots:
    void onAppleFadeOutFinished(); // 新增：处理苹果淡出完成
};

// ==================== 【新增：拯救苹果专属片段着色器】 ====================
// ==================== 【精简版：去水珠 + 强化潮湿感着色器】 ====================
const QString APPLE_FRAG_SHADER = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;
uniform float iTime;
uniform vec2 iResolution;
uniform sampler2D u_BgTexture;

// 高精度值噪声
float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = fract(sin(dot(i, vec2(12.9898, 78.233))) * 43758.5453);
    float b = fract(sin(dot(i + vec2(1.0, 0.0), vec2(12.9898, 78.233))) * 43758.5453);
    float c = fract(sin(dot(i + vec2(0.0, 1.0), vec2(12.9898, 78.233))) * 43758.5453);
    float d = fract(sin(dot(i + vec2(1.0, 1.0), vec2(12.9898, 78.233))) * 43758.5453);
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
float fbm(vec2 p) {
    float f = 0.0;
    f += 0.5000 * valueNoise(p); p *= 2.02;
    f += 0.2500 * valueNoise(p); p *= 2.03;
    f += 0.1250 * valueNoise(p); p *= 2.01;
    f += 0.0625 * valueNoise(p);
    return f;
}
mat2 rotate2d(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}
float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = vTexCoord;
    vec2 originalUv = uv; 
    float aspect = iResolution.x / iResolution.y; 

    float leafAreaMask = 1.0 - smoothstep(0.3, 0.8, uv.y); 
    float grassAreaMask = smoothstep(0.5, 0.8, uv.y); 
    float farAreaMask = smoothstep(0.2, 0.7, abs(uv.y - 0.5)); 

    float windTime = iTime * 0.3;
    vec2 windDir = vec2(cos(windTime * 0.2), sin(windTime * 0.15)) * 0.5;
    float windStrength = 0.6 + 0.4 * sin(windTime);

    // [保留] 树叶自然摆动
    vec2 leafNoise1 = vec2(fbm(uv * 8.0 + windTime * 0.8), fbm(uv * 8.0 + windTime * 0.8 + 10.0));
    vec2 leafNoise2 = vec2(fbm(uv * 15.0 + windTime * 1.2), fbm(uv * 15.0 + windTime * 1.2 + 20.0));
    vec2 leafOffset = (leafNoise1 * 0.012 + leafNoise2 * 0.004) * leafAreaMask * windStrength;
    leafOffset += rotate2d(sin(windTime) * 0.05) * leafOffset;

    // [保留] 改进后的多重FBM草地摆动
    vec2 grassNoise1 = vec2(fbm(uv * 12.0 + windTime * 0.9), fbm(uv * 12.0 + windTime * 0.9 + 20.0));
    vec2 grassNoise2 = vec2(fbm(uv * 20.0 + windTime * 1.4), fbm(uv * 20.0 + windTime * 1.4 + 30.0));
    vec2 grassOffset = (grassNoise1 * 0.01 + grassNoise2 * 0.005) * grassAreaMask * windStrength;
    grassOffset += rotate2d(sin(windTime * 1.1) * 0.04) * grassOffset;
    grassOffset *= uv.y * 1.1; 

    // [修改] 移除了 dropOffset (水珠折射扭曲)
    vec2 finalUv = originalUv + leafOffset + grassOffset;
    
    // 强制限制 UV，防黑边
    finalUv = clamp(finalUv, 0.001, 0.999);
    vec4 bgColor = texture(u_BgTexture, finalUv);

    // =======================================================
    // [新增] 潮湿滤镜处理 (Color Grading)
    // 模拟雨天物体被打湿后的深沉感：稍微增强对比度，并压低整体亮度，融入冷色系
    bgColor.rgb = mix(bgColor.rgb, bgColor.rgb * bgColor.rgb, 0.2); 
    bgColor.rgb *= vec3(0.92, 0.96, 1.0); // 整体偏微蓝的雨天冷色调
    // =======================================================

    // 微弱的阳光透过云层 (压低了亮度以符合雨天)
    vec2 sunPos = vec2(0.88, 0.05); 
    vec2 rayDir = normalize(sunPos - originalUv);
    float sunDistance = length(originalUv - sunPos);
    float lightRay = 1.0 - smoothstep(0.0, 0.9, sunDistance);
    float rayAngle = max(0.0, dot(rayDir, vec2(0.0, 1.0)));
    lightRay *= pow(rayAngle, 2.0);
    lightRay *= fbm(vec2(originalUv.y * 8.0, windTime * 0.6)) * 0.6 + 0.4;
    lightRay *= 1.0 - smoothstep(0.7, 0.8, originalUv.y); 
    lightRay = clamp(lightRay, 0.0, 1.0);
    vec3 rayColor = vec3(0.9, 0.93, 0.8);
    bgColor.rgb += rayColor * lightRay * 0.15; // 降低透光强度

    // 空气中悬浮的微小水汽粒子/反光点
    float totalDots = 0.0;
    for(int i = 0; i < 3; i++) {
        vec2 dotUv = originalUv * (4.0 + float(i) * 2.0);
        dotUv += windTime * (0.2 + float(i) * 0.1) * windDir;
        vec2 dotId = floor(dotUv);
        vec2 dotFract = fract(dotUv) - 0.5;
        float dotRandom = valueNoise(dotId + float(i) * 10.0);
        float dotSize = mix(0.02, 0.08, dotRandom);
        float dotBright = mix(0.3, 0.8, dotRandom);
        float dot = smoothstep(dotSize, 0.0, length(dotFract));
        dot *= leafAreaMask;
        dot *= 0.5 + 0.5 * sin(windTime * 2.0 + dotRandom * 10.0);
        totalDots += dot * dotBright;
    }
    bgColor.rgb += vec3(1.0, 0.95, 0.9) * totalDots * 0.4;

    // =======================================================
    // [强化] 水汽感 (Fog)
    // 增加底层雾气浓度，营造阴雨连绵的湿润空气感
    float fogNoise = fbm(originalUv * 2.0 + vec2(windTime * 0.1, 0.0));
    float fogDensity = mix(0.1, 0.35, farAreaMask * fogNoise); // 提升了起雾的浓度
    vec3 fogColor = vec3(0.85, 0.90, 0.95); // 雾气改为偏冷的灰蓝色
    bgColor.rgb = mix(bgColor.rgb, fogColor, fogDensity);
    // =======================================================

    // 阴天云层阴影
    float cloudShadow = fbm(vec2(originalUv.x * 2.0 + windTime * 0.06, originalUv.y * 1.5)) * 0.15;
    bgColor.rgb -= cloudShadow * 0.8;

    // [保留] 高速且错落有致的多层雨线
    float rainSpeed = 8.5; 
    float rainDensity = 0.45; 
    float windForce = 0.6 + windDir.x * 0.8; 
    
    vec2 aspectUv = originalUv * vec2(aspect, 1.0);
    float totalRain = 0.0;
    
    for(int i = 0; i < 4; i++) {
        float fi = float(i);
        float layerScale = 5.0 + fi * 7.0; 
        float layerSpeed = rainSpeed * (1.0 - fi * 0.15); 
        float layerOpacity = 0.7 - fi * 0.15; 

        vec2 rainUv = aspectUv * layerScale;
        rainUv.x += windForce * rainUv.y;
        rainUv.y -= iTime * layerSpeed + hash21(vec2(fi)) * 100.0; 

        vec2 gridId = floor(rainUv);
        vec2 gridFract = fract(rainUv);
        float randomPos = hash21(gridId);
        
        if(hash21(gridId + 30.0) > rainDensity) continue;

        float rainLength = mix(0.1, 0.5, hash21(gridId + 10.0));
        float rainBright = mix(0.5, 1.0, hash21(gridId + 20.0)) * layerOpacity;

        float shapeY = smoothstep(0.0, rainLength * 0.2, gridFract.y) * smoothstep(rainLength, rainLength * 0.1, gridFract.y);
        float shapeX = smoothstep(0.02, 0.0, abs(gridFract.x - randomPos));

        totalRain += shapeX * shapeY * rainBright;
    }
    
    // 渲染雨水本身
    bgColor.rgb = mix(bgColor.rgb, vec3(0.85, 0.92, 1.0), totalRain * 0.6);
    
    FragColor = vec4(bgColor.rgb, 1.0);
}
)";
// ====================================================================
