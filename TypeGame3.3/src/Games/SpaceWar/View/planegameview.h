/* ------------------------------------------------------------------
// 文件名     : planegameview.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 太空大战视图层，负责渲染战机、基地及游戏内特效动画
------------------------------------------------------------------ */
#pragma once
#include "gameviewbase.h"
#include "planegamedata.h"
#include "singleimagebackground.h" 
#include "playerentity.h" // 【新增】引入玩家战机实体
#include <QOpenGLWidget> 
#include <QKeyEvent> // 【新增】引入按键事件

// 在文件顶部的 include 区域，新增引用：
#include "spriteanimation.h"
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QVariantAnimation> 

// 在 class PlaneGameView 定义之前，新增结构体：
struct ExplosionInstance {
    QPointF pos;                 // 爆炸的中心点坐标
    SpriteAnimation* animation;  // 独立的动画控制器
};

class BulletEntity; // 追踪子弹实体前置声明
class MachineGunBulletEntity; //机枪子弹实体前置声明

class PlaneGameView : public GameViewBase
{
    Q_OBJECT
public:
    explicit PlaneGameView(PlaneGameData* data, QObject* parent = nullptr);
    ~PlaneGameView() override;

    // 新增：动态更新分辨率，不销毁场景现有实体
    void updateResolution(int width, int height);

    // 场景接口实现
    void initScene(int width, int height) override;
    void clearScene() override;

    // 飞机渲染管理
    void addPlaneToScene(PlaneEntity* plane);
    void removePlaneFromScene(PlaneEntity* plane); // 从场景中移除飞机图元，但不删除对象，交由Controller统一管理生命周期
    void updateAllPlanes();

    // 重写动画更新接口
    void updateAnimations(float deltaTime) override;

    // OpenGL 控件绑定接口
    void setGLWidget(QOpenGLWidget* glWidget) { m_glWidget = glWidget; }

    // 提供一个获取玩家实体的接口，后续 Controller 获取位置和做碰撞检测会用到
    PlayerEntity* getPlayer() const { return m_playerEntity; }

    // 提供给 Controller 获取当前按键状态的接口
    bool isLeftPressed() const { return m_isLeftPressed; }
    bool isRightPressed() const { return m_isRightPressed; }
    bool isUpPressed() const { return m_upPressed; }    //
    bool isDownPressed() const { return m_downPressed; } //
    bool isSpacePressed() const { return m_isSpacePressed; }

	// 飞机发射追踪子弹的接口，Controller 调用时会创建 BulletEntity 实例并传入
    void addBulletToScene(BulletEntity* bullet);
    void removeBulletFromScene(BulletEntity* bullet);

    // 机炮子弹的渲染接口
    void addMachineGunBulletToScene(MachineGunBulletEntity* bullet);
    void removeMachineGunBulletFromScene(MachineGunBulletEntity* bullet);

    // 播放任务完成特效
    void playMissionSuccessAnimation();
	void playBossWarningAnimation(); // 【核心新增】Boss警告动画接口

public slots:
    void onDataChanged(int dataType) override;
    void onStateChanged(int state) override;
    void onLevelChanged(int level); // 【核心新增】监听关卡变化

signals:
    // 弹窗按钮点击信号
    void nextLevelClicked();
    void restartGameClicked();
    void exitGameClicked(); 

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override; // 前景层绘制：用于渲染顶层弹窗，不会被Item覆盖
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override; // 重写鼠标事件处理弹窗点击

    // 【新增】重写键盘按下和抬起事件
    void keyPressEvent(QKeyEvent* event) override; 
    void keyReleaseEvent(QKeyEvent* event) override;
    // 【新增】：重写焦点丢失事件，防止按键锁死
    void focusOutEvent(QFocusEvent* event) override;

private:
    PlaneGameData* getGameData() { return static_cast<PlaneGameData*>(m_gameData); }

    void initPlayer();    // 【新增】玩家战机初始化方法
    PlayerEntity* m_playerEntity = nullptr;         // 【新增】玩家战机实体指针
    // 【新增】记录左右方向键的真实物理状态
    bool m_isLeftPressed = false;
    bool m_isRightPressed = false;
    bool m_upPressed = false;
    bool m_downPressed = false;
	bool m_isSpacePressed = false; // 记录空格键状态，控制机炮射击

    // 弹窗绘制方法
    void drawLevelCompleteDialog(QPainter* painter); // 绘制关卡完成弹窗
    void drawGameOverDialog(QPainter* painter); // 绘制游戏结束弹窗

    // 按钮区域（用于点击检测）
    QRectF m_nextLevelBtnRect;
    QRectF m_restartBtnRect;
    QRectF m_exitBtnRect;

    // ============== 渲染效果 ================ //
    SingleImageBackground* m_shaderBackground = nullptr; // 新增：使用单图背景类来实现着色器效果，简化渲染逻辑
    QOpenGLWidget* m_glWidget = nullptr; // 所属OpenGL控件
    // 后续扩展渲染效果

    // ==========================================
    // 【核心修改】双轨爆炸特效相关资源与容器
    // ==========================================
    QList<ExplosionInstance> m_activeExplosions; // 正在播放的爆炸特效列表
    QVector<QPixmap> m_explosionFramesNormal;    // 外星母舰爆炸帧 (Stage 0)
    QVector<QPixmap> m_explosionFramesZerg;      // 虫巢异虫爆炸帧 (Stage 1)
    QVector<QPixmap> m_explosionFramesSpace;     // 敌占区普通飞机爆炸帧 (Stage 2)
    void initExplosionResource();                // 初始化：加载并切割爆炸精灵图
    void playExplosionAt(const QPointF& pos, int stage); // 需要传入具体的 stage，以应用不同的爆炸火花
    int m_currentStage = 0; // 0:外星母舰, 1:虫巢, 2:敌占区
    // ==========================================

    // ==========================================
    // 剧情漫画播放
    // ==========================================
    qreal m_mangaScrollProgress = 0.0;      // 漫画滚屏进度 (0.0 到 1.0)
    QVariantAnimation* m_mangaScrollAnim = nullptr; // 自动滚屏动画控制器


private slots:
    void onPlaneFadeOutFinished(); // 处理飞机淡出完成
};

// ==================== 【阶段一：敌占区 (废土战火、探照灯、硝烟)】 ====================
const QString STAGE1_EARTH_SHADER = R"(
    #version 330 core
    in vec2 vTexCoord;
    out vec4 FragColor;

    uniform sampler2D u_BgTexture;
    uniform float iTime;
    uniform vec2 iResolution;
    uniform vec2 iTexResolution;

    float random(vec2 st) { return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123); }
    float hash(vec2 p) {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return fract(p.x * p.y);
    }
    float noise(vec2 st) {
        vec2 i = floor(st); vec2 f = fract(st);
        vec2 u = f * f * (3.0 - 2.0 * f);
        return mix(mix(random(i + vec2(0.0,0.0)), random(i + vec2(1.0,0.0)), u.x),
                   mix(random(i + vec2(0.0,1.0)), random(i + vec2(1.0,1.0)), u.x), u.y);
    }
    float fbm(vec2 st) {
        float value = 0.0; float amplitude = 0.5;
        vec2 shift = vec2(100.0);
        mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
        for (int i = 0; i < 4; ++i) { 
            value += amplitude * noise(st);
            st = rot * st * 2.0 + shift;
            amplitude *= 0.5;
        }
        return value;
    }

    float drawAttachedSearchLight(vec2 bgUV, vec2 baseUV, float angle, float width) {
        vec2 localUV = vec2(bgUV.x, fract(bgUV.y));
        float totalIntensity = 0.0;
        for (float offsetY = -1.0; offsetY <= 1.0; offsetY += 1.0) {
            vec2 diff = localUV - vec2(baseUV.x, baseUV.y + offsetY);
            diff.x *= iResolution.x / iResolution.y; 
            diff.y *= iTexResolution.y / iResolution.y;
            float dist = length(diff);
            if (dist == 0.0) continue; 
            vec2 dir = diff / dist;
            vec2 targetDir = vec2(cos(angle), sin(angle));
            float alignment = dot(dir, targetDir);
            float edgeBlur = 0.005 + dist * 0.02; 
            float beam = smoothstep(1.0 - width - edgeBlur, 1.0 - width + edgeBlur, alignment);
            beam *= smoothstep(0.01, 0.06, dist); 
            float coreGlow = smoothstep(0.02, 0.0, dist) * 1.5; 
            float haloGlow = smoothstep(0.08, 0.0, dist) * 0.4; 
            float intensity = beam + coreGlow + haloGlow;
            float tailFade = 1.0 - smoothstep(0.3, 1.8, dist);
            intensity *= tailFade;
            totalIntensity += intensity;
        }
        return totalIntensity;
    }

    vec4 drawSparseEmbers(vec2 bgUV, vec2 baseUV) {
        vec2 localUV = vec2(bgUV.x, fract(bgUV.y));
        vec4 result = vec4(0.0);
        for (float offsetY = -1.0; offsetY <= 1.0; offsetY += 1.0) {
            vec2 diff = localUV - vec2(baseUV.x, baseUV.y + offsetY);
            diff.x *= iResolution.x / iResolution.y; 
            diff.y *= iTexResolution.y / iResolution.y;
            vec2 emberUV = diff * 20.0;
            emberUV.y += iTime * 3.5; 
            emberUV.x += sin(emberUV.y * 1.5 + iTime * 3.0) * 0.8; 
            vec2 id = floor(emberUV);
            vec2 f = fract(emberUV);
            vec2 p = vec2(hash(id), hash(id + 11.0));
            float emberDist = length(f - p);
            float blink = sin(iTime * 10.0 + hash(id) * 6.28) * 0.5 + 0.5;
            float emberSize = hash(id + 22.0) * 0.05; 
            float sparsity = step(0.85, hash(id + 33.3)); 
            float emberMask = smoothstep(0.05, -0.4, diff.y); 
            emberMask *= smoothstep(0.15, 0.0, abs(diff.x) + diff.y * 0.3); 
            float ember = smoothstep(emberSize, 0.0, emberDist) * emberMask * blink * sparsity;
            vec3 colCore = vec3(1.0, 0.8, 0.2); 
            vec3 colEdge = vec3(1.0, 0.3, 0.0); 
            vec3 emberCol = mix(colEdge, colCore, ember * 1.5);
            result += vec4(emberCol, ember * 1.5);
        }
        return result;
    }

    void main() {
        vec2 bgUV = vTexCoord;
        float screenAspect = iResolution.y / iResolution.x;
        float texAspect = iTexResolution.x / iTexResolution.y;
        bgUV.y *= screenAspect * texAspect; 
        bgUV.y -= iTime * 0.02;      
        vec4 bgColor = texture(u_BgTexture, bgUV);
        vec2 effectUV = vTexCoord;
        effectUV.y *= screenAspect;

        vec2 base1 = vec2(0.33, 0.67); 
        vec2 base2 = vec2(0.80, 0.30); 
        float sweepAngle = sin(iTime * 1.0) * 0.6; 
        float light1 = drawAttachedSearchLight(bgUV, base1, -1.57 + sweepAngle, 0.015);
        float light2 = drawAttachedSearchLight(bgUV, base2, -1.57 - sweepAngle, 0.012);
        vec3 lightColor = vec3(0.85, 0.85, 0.6); 
        float totalLight = (light1 + light2);
        
        vec4 ember1 = drawSparseEmbers(bgUV, base1); 
        vec4 ember2 = drawSparseEmbers(bgUV, base2);  
        vec4 ember3 = drawSparseEmbers(bgUV, vec2(0.45, 0.5));           
        vec3 emberColorSum = ember1.rgb * ember1.a + ember2.rgb * ember2.a + ember3.rgb * ember3.a;

        bgColor.rgb += lightColor * totalLight * 0.6;
        bgColor.rgb += emberColorSum; 

        vec2 fogUV = effectUV * 1.2; 
        vec2 move1 = vec2(iTime * 0.06, iTime * 0.12); 
        vec2 move2 = vec2(iTime * 0.03, iTime * 0.20); 
        float q = fbm(fogUV + move1);
        float fogFactor = fbm(fogUV + move2 + q);
        vec3 smokeColorDark = vec3(0.18, 0.18, 0.20);
        vec3 smokeColorLight = vec3(0.55, 0.55, 0.60);
        vec3 litSmokeColor = smokeColorLight + lightColor * totalLight * 0.7; 
        vec3 finalSmokeColor = mix(smokeColorDark, litSmokeColor, fogFactor);

        float alpha = smoothstep(0.15, 0.85, fogFactor); 
        vec3 finalColor = mix(bgColor.rgb, finalSmokeColor, alpha * 0.75);

        // 已移除：飘雪特效逻辑

        FragColor = vec4(finalColor, 1.0);
    }
)";

    // ==================== 【阶段二：异虫虫巢 (动态蠕动、变色孢子、变色毒瘴)】 ====================
    const QString STAGE2_ZERG_SHADER = R"(
    #version 330 core
    in vec2 vTexCoord;
    out vec4 FragColor;

    uniform sampler2D u_BgTexture;
    uniform float iTime;
    uniform vec2 iResolution;
    uniform vec2 iTexResolution;

    float hash(vec2 p) {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return fract(p.x * p.y);
    }

    float noise(vec2 st) {
        vec2 i = floor(st); vec2 f = fract(st);
        vec2 u = f * f * (3.0 - 2.0 * f);
        return mix(mix(hash(i + vec2(0.0,0.0)), hash(i + vec2(1.0,0.0)), u.x),
                   mix(hash(i + vec2(0.0,1.0)), hash(i + vec2(1.0,1.0)), u.x), u.y);
    }

    float fbm(vec2 st) {
        float value = 0.0; float amplitude = 0.5;
        vec2 shift = vec2(100.0);
        mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
        for (int i = 0; i < 4; ++i) { 
            value += amplitude * noise(st);
            st = rot * st * 2.0 + shift;
            amplitude *= 0.5;
        }
        return value;
    }

    // 【新增】色彩循环函数：在剧毒绿、幽能紫、猩红血之间平滑过渡
    vec3 getZergColorCycle(float t, float offset) {
        float phase = fract(t * 0.15 + offset); // 控制颜色渐变的速度
        vec3 c1 = vec3(0.1, 0.7, 0.2); // 绿
        vec3 c2 = vec3(0.6, 0.1, 0.7); // 紫
        vec3 c3 = vec3(0.8, 0.1, 0.2); // 红
        
        if (phase < 0.333) return mix(c1, c2, smoothstep(0.0, 0.333, phase));
        if (phase < 0.666) return mix(c2, c3, smoothstep(0.333, 0.666, phase));
        return mix(c3, c1, smoothstep(0.666, 1.0, phase));
    }

    // 绘制发光变色的剧毒孢子
    vec4 drawSpores(vec2 uv, float scale, float speed, float density) {
        uv *= scale;
        uv.x += sin(uv.y * 0.5 + iTime) * 0.5; // S型漂浮轨迹
        uv.y += iTime * speed; 
        
        vec2 id = floor(uv); 
        vec2 f = fract(uv);
        if (hash(id) > density) return vec4(0.0); // 控制孢子密度

        vec2 p = vec2(hash(id + 11.0), hash(id + 13.0));
        p.x += sin(iTime * 1.5 + hash(id) * 6.28) * 0.3; 
        p.y += cos(iTime * 1.2 + hash(id) * 6.28) * 0.3; 
        
        float d = length(f - p);
        float baseSize = hash(id + 22.0) * 0.3 + 0.1;
        float pulse = sin(iTime * 4.0 + hash(id) * 10.0) * 0.5 + 0.5; // 呼吸闪烁
        float sizeScale = baseSize * (0.8 + 0.4 * pulse);
        
        float core = 1.0 - smoothstep(0.0, sizeScale * 0.1, d);
        float glow = (1.0 - smoothstep(0.0, sizeScale * 0.4, d)) * 0.6 * pulse;
        
        // 【修改】让每个孢子的颜色产生随机的时间偏移，在绿、紫、红中交替
        vec3 baseColor = getZergColorCycle(iTime, hash(id) * 10.0);
        vec3 sporeColor = mix(baseColor * 0.7, baseColor * 1.3, pulse); // 闪烁时提亮
        
        return vec4(sporeColor, core + glow);
    }

    void main() {
        vec2 uv = vTexCoord;
        float screenAspect = iResolution.y / iResolution.x;
        float texAspect = iTexResolution.x / iTexResolution.y;
        
        // ----------------------------------------------------
        // 1. 底层：完整保留背景图，叠加丰富无规律的生物蠕动感
        // ----------------------------------------------------
        vec2 bgUV = uv;
        bgUV.y *= screenAspect * texAspect; 
        bgUV.y -= iTime * 0.03;      
        
        // 【核心修改】多重三角函数干涉，呈现非线性的肉壁收缩与膨胀
        float periX = sin(bgUV.y * 12.0 + iTime * 2.0) * cos(bgUV.x * 8.0 - iTime * 1.5);
        float periY = cos(bgUV.x * 15.0 + bgUV.y * 20.0 + iTime * 3.0);
        bgUV.x += (periX + periY) * 0.0035;
        bgUV.y += (periX - periY) * 0.0035;

        vec4 baseBg = texture(u_BgTexture, bgUV);
        vec3 finalColor = baseBg.rgb; // 100% 继承原图

        vec2 effectUV = uv;
        effectUV.y *= screenAspect;

        // ----------------------------------------------------
        // 2. 中层：异虫紫色的生物组织脉络 (血管/神经网)
        // ----------------------------------------------------
        vec2 veinUV = effectUV * 3.0;
        veinUV.y -= iTime * 0.1;
        float n = fbm(veinUV + fbm(veinUV * 2.0 - iTime * 0.2));
        
        float veinMask = smoothstep(0.4, 0.45, abs(n - 0.5)) * 0.3; 
        float veinPulse = sin(iTime * 3.0 + n * 10.0) * 0.5 + 0.5; // 脉络的血液搏动感
        vec3 veinColor = vec3(0.4, 0.1, 0.5) * veinMask * veinPulse; 

        // ----------------------------------------------------
        // 3. 气层：纯加法混合的变色毒瘴
        // ----------------------------------------------------
        vec2 fogUV = effectUV * 2.0; 
        float fog1 = fbm(fogUV + vec2(iTime * 0.1, iTime * 0.2));
        float fog2 = fbm(fogUV * 2.0 - vec2(iTime * 0.05, -iTime * 0.1) + fog1);
        
        // 【修改】毒瘴色调随时间大范围变化
        vec3 toxicColor = getZergColorCycle(iTime, 0.0); 
        vec3 fogColor = toxicColor * fog2 * 0.45; // 稍微增加毒气浓稠度

        // ----------------------------------------------------
        // 4. 表层：高亮漂浮的变色孢子群
        // ----------------------------------------------------
        vec4 spores1 = drawSpores(effectUV, 12.0, 0.4, 0.3);
        vec4 spores2 = drawSpores(effectUV, 25.0, 0.6, 0.2);
        vec4 spores3 = drawSpores(effectUV, 40.0, 0.9, 0.1);
        vec4 totalSpores = spores1 + spores2 * 0.8 + spores3 * 0.5;

        // ----------------------------------------------------
        // 5. 加法混合合成
        // ----------------------------------------------------
        finalColor += veinColor;                             // 叠加紫色脉络发光
        finalColor += fogColor;                              // 叠加交替变色毒气
        finalColor += totalSpores.rgb * totalSpores.a * 1.5; // 叠加异色孢子荧光 (提亮 1.5 倍)

        // 四周暗角，加深虫巢内部的压抑感
        float vignette = length(vTexCoord - 0.5) * 1.3;
        vignette = smoothstep(0.4, 1.2, vignette);
        finalColor -= vignette * 0.2; 
        
        // 附加一层与毒瘴匹配的极微弱环境反射光
        finalColor += toxicColor * 0.05 * (1.0 - vignette);

        FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
    }
)";

    // ==================== 【阶段三：外星母舰 (背景透出、蓝色电磁力场盾、数据流)】 ====================
    const QString STAGE3_SPACE_SHADER = R"(
    #version 330 core
    in vec2 vTexCoord;
    out vec4 FragColor;

    uniform sampler2D u_BgTexture;
    uniform float iTime;
    uniform vec2 iResolution;
    uniform vec2 iTexResolution;

    // 工具函数：生成六边形网格
    float hexDist(vec2 p) {
        p = abs(p);
        float c = dot(p, normalize(vec2(1.0, 1.732)));
        return max(c, p.x);
    }

    vec4 hexCoords(vec2 uv) {
        vec2 r = vec2(1.0, 1.732);
        vec2 h = r * 0.5;
        vec2 a = mod(uv, r) - h;
        vec2 b = mod(uv - h, r) - h;
        vec2 gv = dot(a, a) < dot(b, b) ? a : b;
        vec2 id = uv - gv;
        return vec4(gv.x, gv.y, id.x, id.y);
    }

    float hash(vec2 p) {
        return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    }

    void main() {
        vec2 uv = vTexCoord;
        float screenAspect = iResolution.y / iResolution.x;
        float texAspect = iTexResolution.x / iTexResolution.y;

        // ----------------------------------------------------
        // 1. 底层：完全保留并缓慢滚动原始背景图
        // ----------------------------------------------------
        vec2 bgUV = uv;
        bgUV.y *= screenAspect * texAspect; 
        bgUV.y -= iTime * 0.05; 
        
        vec4 baseBg = texture(u_BgTexture, bgUV);
        vec3 finalColor = baseBg.rgb; // 100% 继承你的原图颜色作为基底

        // ----------------------------------------------------
        // 2. 中层：纵向涌动的亮蓝色数据流
        // ----------------------------------------------------
        vec2 effectUV = uv;
        effectUV.y *= screenAspect;
        
        vec2 streamUV = effectUV;
        streamUV.y += iTime * 0.3; // 向上快速传输
        vec2 streamId = floor(streamUV * vec2(20.0, 5.0));
        vec2 streamFract = fract(streamUV * vec2(20.0, 5.0));
        
        float isStream = step(0.92, hash(streamId.xx)); // 稀疏的垂直轨道
        float streamGlow = smoothstep(0.1, 0.0, abs(streamFract.x - 0.5)) * isStream;
        float streamPulse = sin(streamUV.y * 10.0 - iTime * 5.0 + hash(streamId)) * 0.5 + 0.5;
        
        // 亮青蓝色的数据流光
        vec3 streamColor = vec3(0.0, 0.7, 1.0) * streamGlow * streamPulse * 0.8;

        // ----------------------------------------------------
        // 3. 表层：绝对防壁 - 蓝色六边形电磁护盾
        // ----------------------------------------------------
        vec2 shieldUV = effectUV;
        shieldUV.y -= iTime * 0.15; // 带有视差差速的悬浮感
        
        vec4 hex = hexCoords(shieldUV * 12.0); // 调整六边形大小
        float hexD = hexDist(hex.xy);
        
        // 护盾边缘的发光网格（线框细致化）
        float hexEdge = smoothstep(0.48, 0.5, hexD) * smoothstep(0.53, 0.49, hexD);
        
        // 扫描线高光：模拟高能雷达扫过的效果
        float scanline = sin(effectUV.y * 10.0 - iTime * 4.0) * 0.5 + 0.5;
        scanline = pow(scanline, 3.0); // 让扫描线边缘变得极度锐利
        
        // 阵列网格的呼吸闪烁脉冲
        float pulse = sin(iTime * 2.0 + hex.z + hex.w) * 0.5 + 0.5;
        
        vec3 shieldBaseColor = vec3(0.0, 0.8, 1.0); // 纯净的科幻蓝
        vec3 shieldGlow = shieldBaseColor * hexEdge * (0.2 + 0.8 * scanline * pulse);

        // ----------------------------------------------------
        // 4. 加法混合合成
        // ----------------------------------------------------
        finalColor += streamColor;       // 直接叠加光效，绝不遮盖背景
        finalColor += shieldGlow * 1.5;  // 叠加护盾光效，拉高发光亮度

        // 边缘微微压暗，将视觉重心聚拢，但不影响主体
        float vignette = length(vTexCoord - 0.5) * 1.2;
        vignette = smoothstep(0.4, 1.2, vignette);
        finalColor -= vignette * 0.15; 

        FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
    }
)";
    
// ============================================ 特效组 ===============================================
// ==================== 炫酷浮动爆炸文字特效 ====================
class BoomTextItem : public QGraphicsObject {
    Q_OBJECT
public:
    BoomTextItem(const QPointF& centerPos, QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent) {
        setPos(centerPos);
        setZValue(200); // 保证在所有飞机和特效的最上层

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

        // 1. 向上漂浮动画
        QPropertyAnimation* moveAnim = new QPropertyAnimation(this, "pos");
        moveAnim->setDuration(600);
        moveAnim->setStartValue(centerPos);
        moveAnim->setEndValue(centerPos + QPointF(0, -80)); // 向上浮动 80 像素
        moveAnim->setEasingCurve(QEasingCurve::OutQuad);
        group->addAnimation(moveAnim);

        // 2. 弹簧放大动画 (极具冲击力)
        QPropertyAnimation* scaleAnim = new QPropertyAnimation(this, "scale");
        scaleAnim->setDuration(400);
        scaleAnim->setStartValue(0.2);
        scaleAnim->setEndValue(1.5);
        scaleAnim->setEasingCurve(QEasingCurve::OutBack); // 带有强烈回弹的放大
        group->addAnimation(scaleAnim);

        // 3. 淡出动画
        QPropertyAnimation* fadeAnim = new QPropertyAnimation(this, "opacity");
        fadeAnim->setDuration(600);
        fadeAnim->setStartValue(1.0);
        fadeAnim->setEndValue(0.0);
        fadeAnim->setEasingCurve(QEasingCurve::InQuad); // 前半段保持，后半段加速消失
        group->addAnimation(fadeAnim);

        // 动画播完后，自动把这个图元从内存中彻底销毁
        connect(group, &QParallelAnimationGroup::finished, this, &QObject::deleteLater);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QRectF boundingRect() const override {
        return QRectF(-100, -50, 200, 100);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::TextAntialiasing);

        // 使用 Impact 这种极具力量感的厚重字体，加上倾斜
        QFont font("Impact", 24, QFont::Black, true);
        painter->setFont(font);

        QPainterPath path;
        QRectF textRect = painter->fontMetrics().boundingRect("BOOM!");
        // 让文字完美居中
        path.addText(-textRect.width() / 2.0, textRect.height() / 3.0, font, "BOOM!");

        // 烈焰渐变填充 (核心黄 -> 边缘红)
        QLinearGradient gradient(0, -30, 0, 30);
        gradient.setColorAt(0.0, QColor(255, 255, 0));
        gradient.setColorAt(0.5, QColor(255, 100, 0));
        gradient.setColorAt(1.0, QColor(200, 0, 0));

        painter->setBrush(gradient);
        painter->setPen(QPen(Qt::white, 3)); // 粗白描边，让文字在任何背景下都极其醒目
        painter->drawPath(path);
    }
};
// ==============================================================

// ==================== 关卡完成炫酷文字特效 ====================
class MissionSuccessTextItem : public QGraphicsObject {
    Q_OBJECT
public:
    MissionSuccessTextItem(const QPointF& centerPos, QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent) {
        setPos(centerPos);
        setZValue(400); // 确保在全屏弹窗黑幕和所有游戏元素之上

        QSequentialAnimationGroup* seqGroup = new QSequentialAnimationGroup(this);

        // 阶段 1：极速砸屏 (带来强烈的机甲物理冲击感)
        QParallelAnimationGroup* enterGroup = new QParallelAnimationGroup(this);
        QPropertyAnimation* scaleIn = new QPropertyAnimation(this, "scale");
        scaleIn->setDuration(250);
        scaleIn->setStartValue(4.0); // 从巨大的尺寸瞬间压缩
        scaleIn->setEndValue(1.0);
        scaleIn->setEasingCurve(QEasingCurve::OutBack); // 强烈的回弹反馈

        QPropertyAnimation* fadeIn = new QPropertyAnimation(this, "opacity");
        fadeIn->setDuration(150);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);

        enterGroup->addAnimation(scaleIn);
        enterGroup->addAnimation(fadeIn);
        seqGroup->addAnimation(enterGroup);

        // 阶段 2：悬停高亮展示，给玩家反应时间
        seqGroup->addPause(1200);

        // 阶段 3：数据流失般地快速消散
        QPropertyAnimation* fadeOut = new QPropertyAnimation(this, "opacity");
        fadeOut->setDuration(300);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        seqGroup->addAnimation(fadeOut);

        connect(seqGroup, &QSequentialAnimationGroup::finished, this, &QObject::deleteLater);
        seqGroup->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QRectF boundingRect() const override {
        return QRectF(-350, -100, 700, 200);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::TextAntialiasing);

        QFont font("Impact", 64, QFont::Black, true); // 极具压迫感的重型斜体
        painter->setFont(font);

        QString text = "MISSION SUCCESS";
        QPainterPath path;
        QRectF textRect = painter->fontMetrics().boundingRect(text);
        path.addText(-textRect.width() / 2.0, textRect.height() / 3.0, font, text);

        // 多重描边渲染：底层电磁蓝光晕 -> 中层亮青色装甲边 -> 核心纯白
        painter->setPen(QPen(QColor(0, 191, 255, 120), 12));
        painter->drawPath(path);

        painter->setPen(QPen(QColor(0, 255, 204), 4));
        painter->drawPath(path);

        painter->setBrush(Qt::white);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
    }
};
// ==============================================================

// ==================== Boss战专属红色警告特效 ====================
class BossWarningTextItem : public QGraphicsObject {
    Q_OBJECT
public:
    BossWarningTextItem(const QPointF& centerPos, QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent) {
        setPos(centerPos);
        setZValue(400); // 确保在最上层

        QSequentialAnimationGroup* seqGroup = new QSequentialAnimationGroup(this);

        // 闪烁警报特效 (闪烁3次)
        for (int i = 0; i < 3; ++i) {
            QPropertyAnimation* flashIn = new QPropertyAnimation(this, "opacity");
            flashIn->setDuration(400);
            flashIn->setStartValue(0.0);
            flashIn->setEndValue(1.0);
            flashIn->setEasingCurve(QEasingCurve::OutQuad);
            seqGroup->addAnimation(flashIn);

            QPropertyAnimation* flashOut = new QPropertyAnimation(this, "opacity");
            flashOut->setDuration(400);
            flashOut->setStartValue(1.0);
            flashOut->setEndValue(0.0);
            flashOut->setEasingCurve(QEasingCurve::InQuad);
            seqGroup->addAnimation(flashOut);
        }

        connect(seqGroup, &QSequentialAnimationGroup::finished, this, &QObject::deleteLater);
        seqGroup->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QRectF boundingRect() const override {
        return QRectF(-350, -100, 700, 200);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::TextAntialiasing);

        QFont font("Impact", 64, QFont::Black, true);
        painter->setFont(font);

        QString text = "WARNING!";
        QPainterPath path;
        QRectF textRect = painter->fontMetrics().boundingRect(text);
        path.addText(-textRect.width() / 2.0, textRect.height() / 3.0, font, text);

        // 致命红晕描边
        painter->setPen(QPen(QColor(255, 0, 0, 150), 12));
        painter->drawPath(path);

        painter->setPen(QPen(QColor(255, 50, 50), 4));
        painter->drawPath(path);

        painter->setBrush(Qt::white);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
    }
};