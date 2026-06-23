# LAppLive2D — 独立 Live2D 渲染库
本文档是将相关代码投喂给AI所生成的，已经过人工审阅，未发现错误。
## 概述

`lapp_live2d` 是对 [Live2D Cubism SDK for Native](https://www.live2d.com/download/cubism-sdk/) 的封装层。  
它将原 SDK 示例代码中的 OpenGL 硬编码抽离为抽象接口，使库本身**不依赖任何窗口框架**（Qt / SDL / GLFW / 自研引擎均可接入）。

**核心设计原则**：
- 零 Qt / GLFW / SDL 依赖 — 仅依赖 C++ 标准库 + OpenGL 头文件 + Cubism SDK
- 渲染后端通过 `IRenderContext` / `ISpriteRenderer` 抽象接口注入
- 窗口尺寸变更通过 `std::function` 回调通知，不持有窗口指针
- 开发者可自由选择窗口框架，编写自己的 `GLCore`

---

## 依赖

| 依赖 | 说明 |
|------|------|
| **Live2D Cubism Framework** | `libFramework.a`（静态库，Live2D 官方） |
| **Live2D Cubism Core** | `libLive2DCubismCore.a`（静态库，Live2D 官方） |
| **OpenGL / OpenGL ES 头文件** | 桌面: `GLEW` + `GLFW`头文件；嵌入式: `EGL` + `GLES2` |
| **C++ 标准库** | `std::function`, `cstdint` 等 |

不依赖任何 Qt / SDL / GLFW 链接库。

---

## 架构

```
┌───────────────────────────────────────────────────────────┐
│  LAppDelegate (单例，应用入口)                              │
│    Initialize(w, h)     — 仅需传入窗口尺寸                   │
│    resize(w, h)         — 窗口变更通知                       │
│    update()             — 每帧渲染                           │
│    SetRenderContext()   — 注入渲染后端                       │
│    SetWindowResizeCallback() — 窗口尺寸变更回调              │
│    NotifyWindowResize() — LApp内部通知应用层调整窗口          │
│    GetWindowWidth/Height() — 获取当前存储的窗口尺寸           │
└──────────────┬────────────────┬────────────────────────────┘
               │ 持有           │ 持有
    ┌──────────▼──────┐   ┌─────▼──────────────────────────┐
    │  LAppView       │   │  LAppLive2DManager             │
    │  渲染管理        │   │  模型生命周期(加载/切换/更新)     │
    │  - 触摸事件      │   │  - OnTap / OnDrag              │
    │  - 坐标变换      │   │  - ModelSizeChange → 回调通知   │
    │  - Sprite绘制    │   │                                │
    └────────┬─────────┘   └─────┬──────────────────────────┘
             │                   │ 管理
    ┌────────▼───────────────────▼──────┐
    │  LAppModel : CubismUserModel      │
    │  单个Live2D模型实例                │
    │  - 加载(.moc3 / .model3.json)     │
    │  - 动画、物理、口型同步            │
    │  - 命中检测 (HitTest)             │
    │  - 渲染 (Draw → CubismRenderer)   │
    └───────────────────────────────────┘
```

### 抽象接口

```
IRenderContext                 ISpriteRenderer
  │                              │
  ├─ Clear(r,g,b,a)              ├─ SetColor(r,g,b,a)
  ├─ ClearDepth(d)               ├─ SetWindowSize(w,h)
  ├─ SetViewport(x,y,w,h)        ├─ RenderImmidiate(texId, uv)
  ├─ CreateShaderProgram()       ├─ IsHit(px, py)
  ├─ GetShaderProgram()          ├─ ResetRect(x,y,w,h)
  ├─ InitializeGLState()         └─ GetTextureId()
  │
  └── GLRenderContext (OpenGL实现)
      - CompileShader() 内部编译 GLSL
      - Clear → glClear / glClearColor
      - SetViewport → glViewport
```

---

## 接入方式（CMake）

```cmake
# 1. 在父 CMakeLists.txt 中配置 Framework 和 Core（IMPORTED）
add_library(Framework STATIC IMPORTED GLOBAL)
set_target_properties(Framework PROPERTIES IMPORTED_LOCATION "/path/to/libFramework.a")

add_library(Live2DCubismCore STATIC IMPORTED GLOBAL)
set_target_properties(Live2DCubismCore PROPERTIES IMPORTED_LOCATION "/path/to/libLive2DCubismCore.a")

# 2. 导入 LAppLive2D 子项目
add_subdirectory(3rdparty/Live2D/Src/LAppLive2D)

# 3. 链接到你的可执行目标
target_link_libraries(your_app PRIVATE lapp_live2d)
```

**注意**：LAppLive2D 内部引用 `<GL/glew.h>`（桌面）或 `<GLES2/gl2.h>`（嵌入式），需确保对应的头文件路径可用。参见父项目的 `CMakeLists.txt` 中如何为 `lapp_live2d` 补充平台 GL 头文件路径。

---

## 编写自定义窗口（以 Qt 为例）

```cpp
#include <QOpenGLWidget>
#include "LAppDelegate.hpp"
#include "GLRenderContext.hpp"

class MyGLCore final : public QOpenGLWidget
{
    void initializeGL() override
    {
        // 1. 注入 IRenderContext（OpenGL 实现）
        LAppDelegate::GetInstance()->SetRenderContext(new GLRenderContext());

        // 2. 注册窗口大小变更回调（模型加载时 LApp 通知你调整窗口）
        LAppDelegate::GetInstance()->SetWindowResizeCallback([this](int w, int h) {
            setFixedSize(w, h);
        });

        // 3. 初始化 LApp（传入当前窗口尺寸）
        LAppDelegate::GetInstance()->Initialize(width(), height());
    }

    void resizeGL(int w, int h) override
    {
        LAppDelegate::GetInstance()->resize(w, h);
    }

    void paintGL() override
    {
        LAppDelegate::GetInstance()->update();
    }

    void mousePressEvent(QMouseEvent *ev) override {
        LAppDelegate::GetInstance()->GetView()->OnTouchesBegan(ev->position().x(), ev->position().y());
    }
    void mouseMoveEvent(QMouseEvent *ev) override {
        LAppDelegate::GetInstance()->GetView()->OnTouchesMoved(ev->position().x(), ev->position().y());
    }
    void mouseReleaseEvent(QMouseEvent *ev) override {
        LAppDelegate::GetInstance()->GetView()->OnTouchesEnded(ev->position().x(), ev->position().y());
    }
};
```

---

## 切换渲染后端

### 当前支持

| 后端 | 宏定义 | 说明 |
|------|--------|------|
| OpenGL ES2 | `RENDER_BACKEND_GLES2` | 嵌入式（RK3566 等） |
| OpenGL | `RENDER_BACKEND_OPENGL` | 桌面 Windows / Linux |

### 添加新后端

**第 1 步** — 在 `LAppOpenGL.hpp` 的宏分支中加一条：

```cpp
#elif defined(RENDER_BACKEND_VULKAN)
  #define RENDERER_BACKEND_TAG Vulkan
```

**第 2 步** — 实现 `VulkanRenderContext`（继承 `IRenderContext`），并在其中实现 `Clear` / `SetViewport` / `CreateShaderProgram` 等方法。

**第 3 步** — CMake 中 `add_definitions(-DRENDER_BACKEND_VULKAN)`。

**第 4 步** — 在你的窗口 `initializeGL()` 等价函数中创建 `VulkanRenderContext` 注入即可。

切换后端时 `LAppModel` 和 `LAppView` 中的 `CUBISM_RENDERER_TYPE` / `CUBISM_OFFSCREEN_TYPE` 宏会自动跟随编译宏切换对应的 Cubism SDK 后端类型。

---

## API 参考

### LAppDelegate（单例）

```cpp
// 初始化（必须在 OpenGL 上下文就绪后调用）
bool Initialize(int windowWidth, int windowHeight);

// 每帧调用
void update();

// 窗口尺寸变更
void resize(int width, int height);

// 注入渲染后端
void SetRenderContext(IRenderContext* ctx);
IRenderContext* GetRenderContext() const;

// 窗口尺寸回调（模型加载时LApp通知应用层调整窗口）
using WindowResizeFunc = std::function<void(int width, int height)>;
void SetWindowResizeCallback(WindowResizeFunc cb);

// 获取当前存储的窗口尺寸
int GetWindowWidth() const;
int GetWindowHeight() const;

// 获取 View / TextureManager
LAppView* GetView();
LAppTextureManager* GetTextureManager();
```

### IRenderContext

```cpp
class IRenderContext {
public:
    virtual ~IRenderContext() = default;
    virtual void Clear(float r, float g, float b, float a) = 0;
    virtual void ClearDepth(float depth) = 0;
    virtual void SetViewport(int x, int y, int w, int h) = 0;
    virtual uintptr_t CreateShaderProgram() = 0;
    virtual uintptr_t GetShaderProgram() const = 0;
    virtual void InitializeGLState() = 0;
};
```

### ISpriteRenderer

```cpp
class ISpriteRenderer {
public:
    virtual ~ISpriteRenderer() = default;
    virtual void SetColor(float r, float g, float b, float a) = 0;
    virtual void SetWindowSize(int w, int h) = 0;
    virtual void RenderImmidiate(uintptr_t textureId, const float uvVertex[8]) const = 0;
    virtual bool IsHit(float px, float py) const = 0;
    virtual void ResetRect(float x, float y, float w, float h) = 0;
    virtual uintptr_t GetTextureId() const = 0;
};
```

---

## 触摸事件映射

LApp 不依赖任何窗口事件系统，触摸由应用层主动调用：

```cpp
// 对应 QMouseEvent / SDL_MouseButtonEvent / GLFW 回调
LAppDelegate::GetInstance()->GetView()->OnTouchesBegan(x, y);  // 按下
LAppDelegate::GetInstance()->GetView()->OnTouchesMoved(x, y);  // 移动
LAppDelegate::GetInstance()->GetView()->OnTouchesEnded(x, y);  // 释放

// 坐标需为窗口内像素坐标，原点在左上角
```

---

## 项目结构

```
LAppLive2D/
├── Inc/
│   ├── LAppDelegate.hpp        ← 应用入口（单例）
│   ├── LAppView.hpp            ← 渲染视图管理
│   ├── LAppModel.hpp           ← 模型实例
│   ├── LAppSprite.hpp          ← Sprite 绘制（继承 ISpriteRenderer）
│   ├── LAppLive2DManager.hpp   ← 模型集管理
│   ├── LAppTextureManager.hpp  ← 纹理管理
│   ├── LAppPal.hpp             ← 平台抽象（文件IO、时间）
│   ├── LAppAllocator.hpp       ← 内存分配器
│   ├── LAppDefine.hpp          ← 配置常量
│   ├── LAppWavFileHandler.hpp  ← WAV文件解析
│   ├── TouchManager.hpp        ← 触摸状态管理
│   ├── LAppOpenGL.hpp          ← OpenGL 头文件 + 后端宏
│   │
│   ├── IRenderContext.hpp      ← 渲染上下文抽象接口 ★
│   ├── ISpriteRenderer.hpp     ← Sprite渲染抽象接口 ★
│   └── GLRenderContext.hpp     ← OpenGL IRenderContext 实现 ★
│
├── Src/
│   ├── *.cpp                   ← 各模块实现
│   └── ...
│
├── CMakeLists.txt              ← 子项目构建脚本
└── README.md                   ← 本文件
```
