/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppLive2DManager.hpp"
#include <stdio.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <io.h>            // 保持 Windows 下继续用旧实现
#include <direct.h>
#else
#include <filesystem>      // Linux / macOS 用 std::filesystem
#include <algorithm>
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Rendering/CubismRenderer.hpp>
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppDelegate.hpp"
#include "LAppModel.hpp"
#include "LAppView.hpp"

using namespace Csm;
using namespace LAppDefine;
using namespace std;

namespace {
    LAppLive2DManager* s_instance = nullptr;

    void FinishedMotion(ACubismMotion* self)
    {
        LAppPal::PrintLogLn("Motion Finished: %x", self);
    }

    int CompareCsmString(const void* a, const void* b)
    {
        return strcmp(reinterpret_cast<const Csm::csmString*>(a)->GetRawString(),
            reinterpret_cast<const Csm::csmString*>(b)->GetRawString());
    }
}

LAppLive2DManager* LAppLive2DManager::GetInstance()
{
    if (s_instance == nullptr)
    {
        s_instance = new LAppLive2DManager();
    }

    return s_instance;
}

void LAppLive2DManager::ReleaseInstance()
{
    if (s_instance != nullptr)
    {
        delete s_instance;
    }

    s_instance = nullptr;
}

LAppLive2DManager::LAppLive2DManager()
    : _viewMatrix(nullptr)
    , _sceneIndex(0)
{
    _viewMatrix = new CubismMatrix44();
    //SetUpModel();
    // Resources/Haru/   Haru.model3.json
    LoadModelFromPath("Resources/Live2DModels/KITU17/", "KITU17.model3.json");      // 默认加载的模型
    //ChangeScene(_sceneIndex);
}

LAppLive2DManager::~LAppLive2DManager()
{
    ReleaseAllModel();
    delete _viewMatrix;
}

void LAppLive2DManager::ReleaseAllModel()
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        delete _models[i];
    }

    _models.Clear();
}

void LAppLive2DManager::SetUpModel()
{
    // ResourcesPathの中にあるフォルダ名を全てクロールし、モデルが存在するフォルダを定義する。
    // フォルダはあるが同名の.model3.jsonが見つからなかった場合はリストに含めない。
    // 遍历ResourcesPath中的所有文件夹名称，定义包含模型的文件夹。
    // 如果文件夹存在但未找到同名的.model3.json文件，则不将其包含在列表中。
     _modelDir.Clear();

#if defined(_WIN32)          // Windows 旧代码
    csmString crawlPath(ResourcesPath);
    crawlPath += "*.*";

    struct _finddata_t fdata;
    intptr_t fh = _findfirst(crawlPath.GetRawString(), &fdata);
    if (fh == -1) return;

    while (_findnext(fh, &fdata) == 0)
    {
        if ((fdata.attrib & _A_SUBDIR) && strcmp(fdata.name, "..") != 0)
        {
            csmString modelJson(ResourcesPath);
            modelJson += fdata.name;
            modelJson.Append(1, '/');
            modelJson += fdata.name;
            modelJson += ".model3.json";

            struct _finddata_t fdata2;
            if (_findfirst(modelJson.GetRawString(), &fdata2) != -1)
                _modelDir.PushBack(csmString(fdata.name));
        }
    }
    qsort(_modelDir.GetPtr(), _modelDir.GetSize(), sizeof(csmString), CompareCsmString);

#else                        // Linux / macOS 代码
    namespace fs = std::filesystem;
    const fs::path resDir(ResourcesPath);
    if (!fs::exists(resDir) || !fs::is_directory(resDir))
        return;

    for (const auto& entry : fs::directory_iterator(resDir))
    {
        if (!entry.is_directory()) continue;

        const std::string& dirName = entry.path().filename().string();
        if (dirName == "." || dirName == "..") continue;

        fs::path modelJson = entry.path() / (dirName + ".model3.json");
        if (fs::exists(modelJson))
            _modelDir.PushBack(csmString(dirName.c_str()));
    }
    // 保持与原代码相同的排序
    qsort(_modelDir.GetPtr(), _modelDir.GetSize(), sizeof(csmString), CompareCsmString);
#endif
}

csmVector<csmString> LAppLive2DManager::GetModelDir() const
{
    return _modelDir;
}

csmInt32 LAppLive2DManager::GetModelDirSize() const
{
    return _modelDir.GetSize();
}

LAppModel* LAppLive2DManager::GetModel(csmUint32 no) const
{
    if (no < _models.GetSize())
    {
        return _models[no];
    }

    return nullptr;
}

/**
 * 处理鼠标拖拽事件
 * @param x
 * @param y
 */
void LAppLive2DManager::OnDrag(csmFloat32 x, csmFloat32 y) const
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        LAppModel* model = GetModel(i);

        model->SetDragging(x, y);
    }
}

/**
 * 处理鼠标点击事件
 * @param x
 * @param y
 * @return
 */
void LAppLive2DManager::OnTap(csmFloat32 x, csmFloat32 y)
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]tap point: {x:%.2f y:%.2f}", x, y);
    }

    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        if (_models[i]->HitTest(HitAreaNameHead, x, y))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("[APP]hit area: [%s]", HitAreaNameHead);
            }
            _models[i]->SetRandomExpression();
        }
        else if (_models[i]->HitTest(HitAreaNameBody, x, y))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("[APP]hit area: [%s]", HitAreaNameBody);
            }

            _models[i]->StartRandomMotion(MotionGroupTapBody, PriorityNormal, FinishedMotion);    // TODO
        }
    }
}

void LAppLive2DManager::OnUpdate() const
{
    int width, height;
    //glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);

    width = LAppDelegate::GetInstance()->GetWindow()->width();
    height = LAppDelegate::GetInstance()->GetWindow()->height();

    csmUint32 modelCount = _models.GetSize();
    for (csmUint32 i = 0; i < modelCount; ++i)
    {
        CubismMatrix44 projection;
        LAppModel* model = GetModel(i);

        if (model->GetModel() == nullptr)
        {
            LAppPal::PrintLogLn("Failed to model->GetModel().");
            continue;
        }

        if (model->GetModel()->GetCanvasWidth() > 1.0f && width < height)
        {
            // 横に長いモデルを縦長ウィンドウに表示する際モデルの横サイズでscaleを算出する
            model->GetModelMatrix()->SetWidth(2.0f);
            projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
        }
        else
        {
            projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f);
        }

        // 必要があればここで乗算
        if (_viewMatrix != nullptr)
        {
            projection.MultiplyByMatrix(_viewMatrix);
        }

        // モデル1体描画前コール
        LAppDelegate::GetInstance()->GetView()->PreModelDraw(*model);

        model->Update();
        model->Draw(projection);///< 参照渡しなのでprojectionは変質する

        // モデル1体描画後コール
        LAppDelegate::GetInstance()->GetView()->PostModelDraw(*model);
    }
}
#include <AppContext.h>
void LAppLive2DManager::LoadModelFromPath(const std::string& modelPath, const std::string& fileName) 
{
    csmString modelPathStr(modelPath.c_str());
    csmString modelJsonName(fileName.c_str());

    ReleaseAllModel();  // 释放当前所有模型 有一个点要注意，在这里先释放然后再Push模型实例，以及加载模型实例
    _models.PushBack(new LAppModel());  // 这样在加载的时候都使用的models[0]这一个位置，自行实现模型选择器要注意注意
    _models[0]->LoadAssets(modelPathStr.GetRawString(), modelJsonName.GetRawString());

    // 加载完后根据模型大小来重新设置当前窗口大小
    const int width = static_cast<int>(_models[0]->GetModel()->GetCanvasWidthPixel() / 15.0);
    const int height = static_cast<int>(_models[0]->GetModel()->GetCanvasHeightPixel() / 15.0);
    AppContext::GetGLCore()->setWindowSize(width, height);
    LAppPal::PrintLogLn("[APP]窗口尺寸重新设置为: W: %d H: %d", width, height);
    /*
     * 提供一个半透明表示模型的示例。
     * 如果定义了USE_RENDER_TARGET或USE_MODEL_RENDER_TARGET，
     * 则将模型绘制到另一个渲染目标上，并将绘制结果作为纹理应用到另一个精灵上。
     */
    {
#if defined(USE_RENDER_TARGET)
        // 如果选择将绘制操作在LAppView持有的目标上进行，则选择此选项
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ViewFrameBuffer;
#elif defined(USE_MODEL_RENDER_TARGET)
        // 如果选择将绘制操作在各个LAppModel持有的目标上进行，则选择此选项
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ModelFrameBuffer;
#else
        // 默认情况下，渲染到主帧缓冲区（通常是直接渲染到屏幕）
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_None;
#endif

#if defined(USE_RENDER_TARGET) || defined(USE_MODEL_RENDER_TARGET)
        // 作为给模型单独设置α（透明度）的示例，创建另一个模型实例并稍微移动其位置
        _models.PushBack(new LAppModel());
        _models[1]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());
        _models[1]->GetModelMatrix()->TranslateX(0.2f);
#endif

        LAppDelegate::GetInstance()->GetView()->SwitchRenderingTarget(useRenderTarget);

        // 当选择其他渲染目标时的背景清除颜色
        float clearColor[3] = { 1.0f, 1.0f, 1.0f };
        LAppDelegate::GetInstance()->GetView()->SetRenderTargetClearColor(clearColor[0], clearColor[1], clearColor[2]);
    }
}


void LAppLive2DManager::NextScene()
{
    csmInt32 no = (_sceneIndex + 1) % GetModelDirSize();
    ChangeScene(no);
}

void LAppLive2DManager::ChangeScene(Csm::csmInt32 index)
{
    _sceneIndex = index;
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]model index: %d", _sceneIndex);
    }

    // model3.jsonのパスを決定する.
    // ディレクトリ名とmodel3.jsonの名前を一致していることが条件
    const csmString& model = _modelDir[index];

    csmString modelPath(ResourcesPath);
    modelPath += model;
    modelPath.Append(1, '/');

    csmString modelJsonName(model);
    modelJsonName += ".model3.json";
    
    ReleaseAllModel();
    _models.PushBack(new LAppModel());
    _models[0]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());

    /*
     * 提供一个半透明表示模型的示例。
     * 如果定义了USE_RENDER_TARGET或USE_MODEL_RENDER_TARGET，
     * 则将模型绘制到另一个渲染目标上，并将绘制结果作为纹理应用到另一个精灵上。
     */
    {
#if defined(USE_RENDER_TARGET)
        // 如果选择将绘制操作在LAppView持有的目标上进行，则选择此选项
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ViewFrameBuffer;
#elif defined(USE_MODEL_RENDER_TARGET)
        // 如果选择将绘制操作在各个LAppModel持有的目标上进行，则选择此选项
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ModelFrameBuffer;
#else
        // 默认情况下，渲染到主帧缓冲区（通常是直接渲染到屏幕）
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_None;
#endif

#if defined(USE_RENDER_TARGET) || defined(USE_MODEL_RENDER_TARGET)
        // 作为给模型单独设置α（透明度）的示例，创建另一个模型实例并稍微移动其位置
        _models.PushBack(new LAppModel());
        _models[1]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());
        _models[1]->GetModelMatrix()->TranslateX(0.2f);
#endif

        LAppDelegate::GetInstance()->GetView()->SwitchRenderingTarget(useRenderTarget);

        // 当选择其他渲染目标时的背景清除颜色
        float clearColor[3] = { 1.0f, 1.0f, 1.0f };
        LAppDelegate::GetInstance()->GetView()->SetRenderTargetClearColor(clearColor[0], clearColor[1], clearColor[2]);
    }
}

/**
 * @brief 启动当前活动模型的唇形同步并播放指定的 WAV 文件
 * @param wavFilePath WAV 文件的路径
 */
void LAppLive2DManager::StartLipSync(const Csm::csmString& wavFilePath)
{
    if (_models.GetSize() == 0)
    {
        LAppPal::PrintLogLn("[APP] Error: No model is loaded.");
        return;
    }
    // TODO Mark
    // 获取当前活动模型（只有一个模型的情况）
    LAppModel* model = _models[0];
    if (model == nullptr)
    {
        LAppPal::PrintLogLn("[APP] Error: Current model is null.");
        return;
    }

    // 调用模型的 StartLipSync 方法
    model->StartLipSync(wavFilePath);
}

/**
 * @brief 启动指定模型的唇形同步并播放指定的 WAV 文件
 * @param modelIndex 模型索引
 * @param wavFilePath WAV 文件的路径
 */
void LAppLive2DManager::StartLipSync(csmUint32 modelIndex, const Csm::csmString& wavFilePath)
{
    if (modelIndex >= _models.GetSize())
    {
        LAppPal::PrintLogLn("[APP] Error: Model index out of range.");
        return;
    }

    // 获取指定模型
    LAppModel* model = _models[modelIndex];
    if (model == nullptr)
    {
        LAppPal::PrintLogLn("[APP] Error: Model is null.");
        return;
    }

    // 调用模型的 StartLipSync 方法
    model->StartLipSync(wavFilePath);
}


csmUint32 LAppLive2DManager::GetModelNum() const
{
    return _models.GetSize();
}

void LAppLive2DManager::SetViewMatrix(CubismMatrix44* m)
{
    for (int i = 0; i < 16; i++) {
        _viewMatrix->GetArray()[i] = m->GetArray()[i];
    }
}
