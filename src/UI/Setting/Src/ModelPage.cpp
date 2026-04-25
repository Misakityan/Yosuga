//
// Created by Administrator on 2025/4/1.
//
#include "ModelPage.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include "ElaMessageBar.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include <QtConcurrent>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QFutureWatcher>
#include "LAppLive2DManager.hpp"

ModelPage::ModelPage(QWidget *parent)
    : BasePage(parent) {
    // 预览窗口标题
    setWindowTitle("ModelPage");


    modelUrlEdit = new ElaLineEdit(this);       // 模型Url Edit
    modelUrlEdit->setFixedWidth(300);
    modelUrlEdit->setPlaceholderText("用于显示当前的模型Url");

    modelChoosePushButton = new ElaPushButton("选择模型", this);
    modelChoosePushButton->setToolTip("选择.model3.json结尾的文件");

    modelUsePushButton = new ElaPushButton("使用模型", this);
    modelUsePushButton->setToolTip("使用选择的模型或Url对应的模型");

    ElaScrollPageArea *modelSetArea = new ElaScrollPageArea(this);
    QHBoxLayout *modelSetLayout = new QHBoxLayout(modelSetArea);
    ElaText *modelSetText = new ElaText("模型设置", this);
    modelSetText->setTextPixelSize(15);
    modelSetLayout->addWidget(modelSetText);
    modelSetLayout->addWidget(modelUrlEdit);
    modelSetLayout->addStretch();
    modelSetLayout->addWidget(modelChoosePushButton);
    modelSetLayout->addWidget(modelUsePushButton);
    modelSetLayout->addSpacing(10);

    // 创建滑块控件和标签
    ElaText *modelSizeText = new ElaText("模型大小比例设置", this);
    modelSizeText->setToolTip("实时调整模型大小");
    modelSizeText->setTextPixelSize(15);
    modelSlider = new ElaSlider(this);          // 滑块(用于设置模型实时大小)
    modelSlider->setRange(0, 99);          // 设置范围
    modelSlider->setValue(85);                      // 设置默认值
    modelSlider->setOrientation(Qt::Horizontal);  // 水平方向
    // 创建独立的区域容器
    ElaScrollPageArea *modelSliderArea = new ElaScrollPageArea(this);
    QHBoxLayout *modelSliderLayout = new QHBoxLayout(modelSliderArea);

    // 添加到布局(加一个标签显示数值)
    ElaText *modelSliderValueText = new ElaText("85%", this);
    modelSliderValueText->setTextPixelSize(14);
    connect(modelSlider, &ElaSlider::valueChanged, this, [modelSliderValueText](const int value){
        modelSliderValueText->setText(QString("%1%").arg(value + 1));
        LAppLive2DManager::GetInstance()->ModelSizeChange(100 - value);       // 实时更新模型大小
    });
    modelSliderLayout->addWidget(modelSizeText);
    modelSliderLayout->addWidget(modelSlider);
    modelSliderLayout->addWidget(modelSliderValueText);
    modelSliderLayout->addStretch();  // 让内容靠左

    connect(modelChoosePushButton, &ElaPushButton::clicked, this, [this]() {
        // 创建对话框对象（使用 heap 分配，由 Qt 对象树管理内存）
        auto *fileDialog = new QFileDialog(this);
        fileDialog->setWindowTitle("选择模型文件");
        fileDialog->setNameFilter("Live2D Model (*.model3.json)");

        // 设置初始目录
        const QString exeDir = QCoreApplication::applicationDirPath();  // 获取当前exe所在目录的本地路径
        fileDialog->setDirectory(exeDir);       // 设置初始目录为当前 exe 所在目录

        // 连接信号：当用户选中文件并点击打开时
        connect(fileDialog, &QFileDialog::fileSelected, this, [this, fileDialog](const QString &file) {
            modelFileUrl = QUrl::fromLocalFile(file);
            if (!modelFileUrl.isEmpty()) {
                const QString t = modelFileUrl.toLocalFile();
                const std::pair<QString, QString> path = this->splitPath(t);
                this->modelFilePathFirst = path.first;
                this->modelFilePathSecond = path.second;
                this->modelUrlEdit->setText(t);
                ElaMessageBar::success(ElaMessageBarType::BottomRight, "模型设置", "模型选择成功", 2000, this);
            }
            // 用完即弃，自动清理内存
            fileDialog->deleteLater();
        });
        // 处理取消的情况（防止内存泄漏）
        connect(fileDialog, &QFileDialog::rejected, fileDialog, &QObject::deleteLater);
        // 显示对话框（非阻塞，不会卡住主界面）
        fileDialog->open();
    });
    connect(modelUsePushButton, &ElaPushButton::clicked, this, [this]() {
        // 模型使用
        if (modelFileUrl.isEmpty()) {
            ElaMessageBar::information(ElaMessageBarType::BottomRight, "模型设置", "似乎并没有选择模型", 800.0, this);
            return;
        }
        // UI 状态设置为加载中
        modelUsePushButton->setEnabled(false); // 禁用使用按钮
        modelUsePushButton->setText("加载中"); // 修改按钮文本
        modelChoosePushButton->setEnabled(false); // 禁用选择按钮

        // 获取路径字符串 (必须按值传递给lambda)
        std::string dir = this->modelFilePathFirst.toStdString();
        std::string filename = this->modelFilePathSecond.toStdString();

        // 启动异步任务
        QFuture<LAppModel *> future = QtConcurrent::run([dir, filename]() -> LAppModel * {
            // 以下代码在子线程执行
            // 创建临时 OpenGL 上下文
            auto *context = new QOpenGLContext();
            // 关键点：设置与全局共享上下文共享 (这样主线程才能看到纹理)
            context->setShareContext(QOpenGLContext::globalShareContext());
            if (!context->create()) {
                delete context;
                return nullptr;
            }
            // 创建离屏表面 (因为子线程没有窗口，需要一个假的绘制表面)
            auto *surface = new QOffscreenSurface();
            surface->setFormat(context->format());
            surface->create();
            // 绑定上下文
            if (!context->makeCurrent(surface)) {
                delete surface;
                delete context;
                return nullptr;
            }
            // 执行真正的耗时加载
            // 调用在 Manager 里新写的函数
            LAppModel *model = LAppLive2DManager::GetInstance()->LoadModelInstance(dir, filename);
            // 清理子线程资源
            context->doneCurrent();
            delete surface;
            delete context;

            return model;
        });

        // 监控任务结束
        auto *watcher = new QFutureWatcher<LAppModel *>();
        connect(watcher, &QFutureWatcher<LAppModel *>::finished, this, [this, watcher]() {
            // 下面的代码在主线程中执行
            LAppModel *newModel = watcher->result();
            if (newModel) {
                // 调用挂载函数，瞬间完成切换
                LAppLive2DManager::GetInstance()->MountLoadedModel(newModel);
                ElaMessageBar::success(ElaMessageBarType::BottomRight, "成功", "模型加载完成", 2000, this);
            } else {
                ElaMessageBar::error(ElaMessageBarType::BottomRight, "错误", "模型加载失败 (OpenGL环境异常)", 2000, this);
            }
            // 恢复 UI
            modelUsePushButton->setEnabled(true);
            modelUsePushButton->setText("使用模型");
            modelChoosePushButton->setEnabled(true);
            watcher->deleteLater();
        });
        // 开始监控
        watcher->setFuture(future);
    });


    QWidget *centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("模型设置");
    QVBoxLayout *centerLayout = new QVBoxLayout(centralWidget);
    centerLayout->addWidget(modelSetArea);
    centerLayout->addWidget(modelSliderArea);
    centerLayout->addStretch();
    centerLayout->setContentsMargins(0, 0, 0, 0);
    addCentralWidget(centralWidget, true, true, 0);
}

// 返回 pair<目录路径, 文件名>
std::pair<QString, QString> ModelPage::splitPath(const QString &fullPath) {
    const QFileInfo fileInfo(fullPath);

    // 获取目录部分（自动处理末尾斜杠）
    QString dirPath = fileInfo.dir().absolutePath() + "/";

    // 获取文件名部分（如果是目录则返回空）
    QString fileName = fileInfo.fileName();

    return {dirPath, fileName};
}

ModelPage::~ModelPage() {
}
