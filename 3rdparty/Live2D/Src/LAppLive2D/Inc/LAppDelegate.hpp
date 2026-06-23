/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "LAppOpenGL.hpp"
#include "LAppAllocator.hpp"
#include "IRenderContext.hpp"
#include <functional>

class LAppView;
class LAppTextureManager;

/**
* @brief   アプリケーションクラス。
*   Cubism SDK の管理を行う。
*/
class LAppDelegate
{
public:
    /**
    * @brief   クラスのインスタンス（シングルトン）を返す。<br>
    *           インスタンスが生成されていない場合は内部でインスタンを生成する。
    *
    * @return  クラスのインスタンス
    */
    static LAppDelegate* GetInstance();

    /**
    * @brief   クラスのインスタンス（シングルトン）を解放する。
    *
    */
    static void ReleaseInstance();

    // 新增
    // resize 由应用层(GLCore::resizeGL)调用，通知LApp窗口尺寸变更
    void resize(int width, int height);

    // 新增
    void update();

    IRenderContext* GetRenderContext() const { return _renderContext; }
    void SetRenderContext(IRenderContext* ctx) { _renderContext = ctx; }

    // 窗口大小变更回调 解耦 AppContext/GLCore 依赖
    // 当模型加载后需要调整窗口大小时，LAppLive2DManager 通过此回调通知应用层
    using WindowResizeFunc = std::function<void(int width, int height)>;
    void SetWindowResizeCallback(WindowResizeFunc cb) { _onResizeWindow = std::move(cb); }
    void NotifyWindowResize(int width, int height) { if (_onResizeWindow) _onResizeWindow(width, height); }

    /**
    * @brief   APPに必要なものを初期化する。
    * @param windowWidth   窗口宽度（像素）
    * @param windowHeight  窗口高度（像素）
    */
    // bool Initialize(GLCore* window);              // 原
    // bool Initialize(QWidget* window);             // 中间解耦版本
    bool Initialize(int windowWidth, int windowHeight); // 完全解耦Qt，仅传入尺寸

    /**
    * @brief   解放する。
    */
    void Release();

    /**
    * @brief   実行処理。
    */
    //void Run();   // Misaki 修改

    /**
    * @brief   OpenGL用 glfwSetMouseButtonCallback用関数。
    *
    * @param[in]       window            コールバックを呼んだWindow情報
    * @param[in]       button            ボタン種類
    * @param[in]       action            実行結果
    * @param[in]       modify
    */
    // void OnMouseCallBack(GLFWwindow* window, int button, int action, int modify);
    void OnMouseCallBack(int button, int action, int modify);

    /**
    * @brief   OpenGL用 glfwSetCursorPosCallback用関数。
    *
    * @param[in]       window            コールバックを呼んだWindow情報
    * @param[in]       x                 x座標
    * @param[in]       y                 x座標
    */
    // void OnMouseCallBack(GLFWwindow* window, double x, double y);
    void OnMouseCallBack(double x, double y);

    /**
    * @brief シェーダーを登録する。
    */
    GLuint CreateShader();

    /**
    * @brief   Window尺寸を取得する。
    */
    // GLCore* GetWindow() { return _window; }     // 原
    // QWidget* GetWindow() { return _window; }     // 中间版本
    int GetWindowWidth() const { return _windowWidth; }    // 完全解耦Qt
    int GetWindowHeight() const { return _windowHeight; }  // 完全解耦Qt

    /**
    * @brief   View情報を取得する。
    */
    LAppView* GetView() { return _view; }

    /**
    * @brief   アプリケーションを終了するかどうか。
    */
    bool GetIsEnd() { return _isEnd; }

    /**
    * @brief   アプリケーションを終了する。
    */
    void AppEnd() { _isEnd = true; }

    LAppTextureManager* GetTextureManager() { return _textureManager; }

private:
    /**
    * @brief   コンストラクタ
    */
    LAppDelegate();

    /**
    * @brief   デストラクタ
    */
    ~LAppDelegate();

    /**
    * @brief   Cubism SDK の初期化
    */
    void InitializeCubism();

    /**
     * @brief   CreateShader内部関数 エラーチェック
     */
    bool CheckShader(GLuint shaderId);

    IRenderContext* _renderContext = nullptr;
    WindowResizeFunc _onResizeWindow;              ///< 窗口大小回调
    LAppAllocator _cubismAllocator;              ///< Cubism SDK Allocator
    Csm::CubismFramework::Option _cubismOption;  ///< Cubism SDK Option
    //GLFWwindow* _window;                         ///< OpenGL ウィンドウ
    // GLCore* _window;                             ///< Misaki 修改 
    // QWidget* _window;                             ///< 使用QWidget基类，解耦GLCore
    LAppView* _view;                             ///< View情報
    bool _captured;                              ///< クリックしているか
    float _mouseX;                               ///< マウスX座標
    float _mouseY;                               ///< マウスY座標
    bool _isEnd;                                 ///< APP終了しているか
    LAppTextureManager* _textureManager;         ///< テクスチャマネージャー

    int _windowWidth;                            ///< Initialize関数で設定したウィンドウ幅
    int _windowHeight;                           ///< Initialize関数で設定したウィンドウ高さ
};

// class EventHandler
// {
// public:
//     /**
//     * @brief   glfwSetMouseButtonCallback用コールバック関数。
//     */
//     static void OnMouseCallBack(GLFWwindow* window, int button, int action, int modify)
//     {
//         LAppDelegate::GetInstance()->OnMouseCallBack(window, button, action, modify);
//     }
//
//     /**
//     * @brief   glfwSetCursorPosCallback用コールバック関数。
//     */
//     static void OnMouseCallBack(GLFWwindow* window, double x, double y)
//     {
//          LAppDelegate::GetInstance()->OnMouseCallBack(window, x, y);
//     }
//
// };
