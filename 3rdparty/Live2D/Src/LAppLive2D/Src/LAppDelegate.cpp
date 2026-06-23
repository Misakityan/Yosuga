/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include <iostream>
#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"

using namespace Csm;
using namespace std;
using namespace LAppDefine;

namespace {
    LAppDelegate* s_instance = NULL;
}

LAppDelegate* LAppDelegate::GetInstance()
{
    if (s_instance == NULL)
    {
        s_instance = new LAppDelegate();
    }

    return s_instance;
}

void LAppDelegate::ReleaseInstance()
{
    if (s_instance != NULL)
    {
        delete s_instance;
    }

    s_instance = NULL;
}

// bool LAppDelegate::Initialize(GLCore* window)
// {
//     if (DebugLogEnable)
//     {
//         LAppPal::PrintLogLn("START");
//     }
//
//     // GLFWの初期化
//     if (glfwInit() == GL_FALSE)
//     {
//         if (DebugLogEnable)
//         {
//             LAppPal::PrintLogLn("Can't initilize GLFW");
//         }
//         return GL_FALSE;
//     }
//
//     // Windowの生成_
//     //_window = glfwCreateWindow(RenderTargetWidth, RenderTargetHeight, "SAMPLE", NULL, NULL);
//     _window = window;   // Misaki 修改
//     if (_window == nullptr)
//     {
//         if (DebugLogEnable)
//         {
//             LAppPal::PrintLogLn("Can't create GLFW window.");
//         }
//         glfwTerminate();
//         return GL_FALSE;
//     }
//
//     // Windowのコンテキストをカレントに設定
//     //glfwMakeContextCurrent(_window);
//     _window->makeCurrent();     // Misaki 修改
//     glfwSwapInterval(1);
//
//     if (glewInit() != GLEW_OK) {
//         if (DebugLogEnable)
//         {
//             LAppPal::PrintLogLn("Can't initilize glew.");
//         }
//         glfwTerminate();
//         return GL_FALSE;
//     }
//
//     //テクスチャサンプリング設定
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//
//     //透過設定
//     glEnable(GL_BLEND);
//     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//
//     // Misaki 修改
//     //コールバック関数の登録
//     //glfwSetMouseButtonCallback(_window, EventHandler::OnMouseCallBack);
//     //glfwSetCursorPosCallback(_window, EventHandler::OnMouseCallBack);
//
//     // ウィンドウサイズ記憶
//     //int width, height;
//     //glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
//     _windowWidth = _window->width();    // Misaki 修改
//     _windowHeight = _window->height();
//
//     //AppViewの初期化
//     _view->Initialize();
//
//     // Cubism SDK の初期化
//     InitializeCubism();
//
//     return GL_TRUE;
// }

// bool LAppDelegate::Initialize(GLCore* window)  // 原
// bool LAppDelegate::Initialize(QWidget* window)  // 中间解耦版本
bool LAppDelegate::Initialize(int windowWidth, int windowHeight) // 完全解耦Qt
{
    if (DebugLogEnable) LAppPal::PrintLogLn("START");
    // _window = window;                          // 不再持有窗口指针
    // if (!_window) return false;
    _windowWidth = windowWidth;
    _windowHeight = windowHeight;

    // OpenGL 初始化（不依赖 glew）
    //  原代码：原始GL调用，已抽象到IRenderContext
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _renderContext->InitializeGLState();

    _view->Initialize();
    InitializeCubism();
    return true;
}

void LAppDelegate::Release()
{
    // Windowの削除
    //glfwDestroyWindow(_window);   // Misaki 修改

    // glfwTerminate();

    delete _textureManager;
    delete _view;

    // リソースを解放
    LAppLive2DManager::ReleaseInstance();

    //Cubism SDK の解放
    CubismFramework::Dispose();
}
/* Msaki 修改 
void LAppDelegate::Run()
{
    //メインループ
    while (glfwWindowShouldClose(_window) == GL_FALSE && !_isEnd)
    {
        int width, height;
        glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
        if( (_windowWidth!=width || _windowHeight!=height) && width>0 && height>0)
        {
            //AppViewの初期化
            _view->Initialize();
            // スプライトサイズを再設定
            _view->ResizeSprite();
            // サイズを保存しておく
            _windowWidth = width;
            _windowHeight = height;

            // ビューポート変更
            glViewport(0, 0, width, height);
        }

        // 時間更新
        LAppPal::UpdateTime();

        // 画面の初期化
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearDepth(1.0);

        //描画更新
        _view->Render();

        // バッファの入れ替え
        glfwSwapBuffers(_window);

        // Poll for and process events
        glfwPollEvents();
    }

    Release();

    LAppDelegate::ReleaseInstance();
}
*/

// 分散run实现的功能
void LAppDelegate::resize(int width, int height)
{
    if ((_windowWidth != width || _windowHeight != height) && width > 0 && height > 0)
    {
        //  先更新尺寸再调_view方法，因为_view内部会通过GetWindowWidth/Height获取
        _windowWidth = width;
        _windowHeight = height;
        //AppViewの初期化
        _view->Initialize();
        // スプライトサイズを再設定
        _view->ResizeSprite();

        // ビューポート変更
        // glViewport(0, 0, width, height);     //  原始GL，改用IRenderContext
        _renderContext->SetViewport(0, 0, width, height);
    }
    else
    {
        // glViewport(0, 0, width, height);     //  原始GL，改用IRenderContext
        _renderContext->SetViewport(0, 0, width, height);
    }
}

// void LAppDelegate::update()
// {
//     // 時間更新
//     LAppPal::UpdateTime();
//
//     // 画面の初期化
//     //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);       // 渲染背景为透明
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glClearDepth(1.0);
//
//     //描画更新
//     _view->Render();
// }
void LAppDelegate::update()
{
    LAppPal::UpdateTime();
    //  原代码：原始GL调用，已抽象到IRenderContext
    // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // LAPP_GL_CLEAR_DEPTH(1.0f);
    _renderContext->Clear(0.0f, 0.0f, 0.0f, 0.0f);
    _renderContext->ClearDepth(1.0f);
    _view->Render();
}

LAppDelegate::LAppDelegate():
    _cubismOption(),
    // _window(nullptr),                          //  已解耦，不再持有窗口指针
    _captured(false),
    _mouseX(0.0f),
    _mouseY(0.0f),
    _isEnd(false),
    _windowWidth(0),
    _windowHeight(0)
{
    _view = new LAppView();
    _textureManager = new LAppTextureManager();
}

LAppDelegate::~LAppDelegate()
{

}

void LAppDelegate::InitializeCubism()
{
    //setup cubism
    _cubismOption.LogFunction = LAppPal::PrintMessage;
    _cubismOption.LoggingLevel = LAppDefine::CubismLoggingLevel;
    _cubismOption.LoadFileFunction = LAppPal::LoadFileAsBytes;
    _cubismOption.ReleaseBytesFunction = LAppPal::ReleaseBytes;
    Csm::CubismFramework::StartUp(&_cubismAllocator, &_cubismOption);

    //Initialize cubism
    CubismFramework::Initialize();

    //load model
    LAppLive2DManager::GetInstance();

    //default proj
    CubismMatrix44 projection;

    LAppPal::UpdateTime();

    _view->InitializeSprite();
}

// void LAppDelegate::OnMouseCallBack(GLFWwindow* window, int button, int action, int modify)
// {
//     if (_view == nullptr)
//     {
//         return;
//     }
//     if (GLFW_MOUSE_BUTTON_LEFT != button)
//     {
//         return;
//     }
//
//     if (GLFW_PRESS == action)
//     {
//         _captured = true;
//         _view->OnTouchesBegan(_mouseX, _mouseY);
//     }
//     else if (GLFW_RELEASE == action)
//     {
//         if (_captured)
//         {
//             _captured = false;
//             _view->OnTouchesEnded(_mouseX, _mouseY);
//         }
//     }
// }
//
// void LAppDelegate::OnMouseCallBack(GLFWwindow* window, double x, double y)
// {
//     _mouseX = static_cast<float>(x);
//     _mouseY = static_cast<float>(y);
//
//     if (!_captured)
//     {
//         return;
//     }
//     if (_view == nullptr)
//     {
//         return;
//     }
//
//     _view->OnTouchesMoved(_mouseX, _mouseY);
// }
void LAppDelegate::OnMouseCallBack(double x, double y)
{
    _mouseX = static_cast<float>(x);
    _mouseY = static_cast<float>(y);
    if (!_captured) return;
    if (_view == nullptr) return;
    _view->OnTouchesMoved(_mouseX, _mouseY);
}

// GLuint LAppDelegate::CreateShader()
// {
//     //バーテックスシェーダのコンパイル
//     GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
//     const char* vertexShader =
//         "#version 120\n"
//         "attribute vec3 position;"
//         "attribute vec2 uv;"
//         "varying vec2 vuv;"
//         "void main(void){"
//         "    gl_Position = vec4(position, 1.0);"
//         "    vuv = uv;"
//         "}";
//     glShaderSource(vertexShaderId, 1, &vertexShader, nullptr);
//     glCompileShader(vertexShaderId);
//     if(!CheckShader(vertexShaderId))
//     {
//         return 0;
//     }
//
//     //フラグメントシェーダのコンパイル
//     GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
//     const char* fragmentShader =
//         "#version 120\n"
//         "varying vec2 vuv;"
//         "uniform sampler2D texture;"
//         "uniform vec4 baseColor;"
//         "void main(void){"
//         "    gl_FragColor = texture2D(texture, vuv) * baseColor;"
//         "}";
//     glShaderSource(fragmentShaderId, 1, &fragmentShader, nullptr);
//     glCompileShader(fragmentShaderId);
//     if (!CheckShader(fragmentShaderId))
//     {
//         return 0;
//     }
//
//     //プログラムオブジェクトの作成
//     GLuint programId = glCreateProgram();
//     glAttachShader(programId, vertexShaderId);
//     glAttachShader(programId, fragmentShaderId);
//
//     // リンク
//     glLinkProgram(programId);
//
//     glUseProgram(programId);
//
//     return programId;
// }
GLuint LAppDelegate::CreateShader()
{
    //  Shader编译逻辑已移入GLRenderContext::CompileShader()
    // 本函数保留兼容签名，内部委托给IRenderContext
    return static_cast<GLuint>(_renderContext->GetShaderProgram());
}
/*
//  原Shader编译代码，已迁移至GLRenderContext::CompileShader()
GLuint LAppDelegate::CreateShader()
{
#if defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_3) || defined(EMBEDDED_LINUX)
    // OpenGL ES 2.0/3.0 着色器
    const char* vertexShader = ...;
    ...
#else
    ...
#endif
    ...
    return programId;
}
*/

// 鼠标回调简化（不再依赖 GLFWwindow*）
void LAppDelegate::OnMouseCallBack(int button, int action, int mods)
{
    if (_view == nullptr) return;
    if (button != 0) return; // 只处理左键，0 代表左键（与 Qt 约定一致）
    if (action == 1) // 按下
    {
        _captured = true;
        _view->OnTouchesBegan(_mouseX, _mouseY);
    }
    else if (action == 0) // 释放
    {
        if (_captured)
        {
            _captured = false;
            _view->OnTouchesEnded(_mouseX, _mouseY);
        }
    }
}

bool LAppDelegate::CheckShader(GLuint shaderId)
{
    GLint status;
    GLint logLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = reinterpret_cast<GLchar*>(CSM_MALLOC(logLength));
        glGetShaderInfoLog(shaderId, logLength, &logLength, log);
        CubismLogError("Shader compile log: %s", log);
        CSM_FREE(log);
    }

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        glDeleteShader(shaderId);
        return false;
    }

    return true;
}
