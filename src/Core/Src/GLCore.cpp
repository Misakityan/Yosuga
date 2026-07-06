#include "LAppDelegate.hpp" // 必须要放在第一个，否则会出现头文件顺序错误
#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppDefine.hpp"
#include "GLCore.h"
#include <QTimer>
#include <QMouseEvent>
#include <QDebug>
#include <QPainter>

#include <QFont>
#include <QApplication>
#include <QFontDatabase>
#include <algorithm>

#include "TextRenderer.h"
#include "AppContext.h"
#include "GLRenderContext.hpp"                     // 渲染后端抽象
#ifdef EMBEDDED_LINUX
#include "AppCore.h"
#include <QIcon>
#endif
QMap<QString, double> GLCore::frameRateMap = {
    {"30", 30.0},
    {"60", 60.0},
    {"120", 120.0},
    {"144", 144.0},
    {"165", 165.0},
    {"240", 240.0}
};

GLCore::GLCore(const int width, const int height, QWidget *parent)
    : QOpenGLWidget(parent),
      isLeftPressed(false),    // 显式初始化
      isRightPressed(false)    // 显式初始化
{
    // 启用高分辨率位图（High DPI Pixmaps）支持
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    //根据实际屏幕缩放比例更改
    qputenv("QT_SCALE_FACTOR", "1.5");
#endif
#endif

    // 不为窗口创建额外的兄弟窗口，从而简化窗口管理并可能提高性能
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    // 设置字体
    QFontDatabase::addApplicationFont("Resources/Font/ElaAwesome.ttf");
    QApplication::setFont(QFont("Microsoft YaHei", 13));

    // new一些必要的对象
#if !defined(EMBEDDED_LINUX)
    contextMenu = new Menu(this);
#endif

    // 设置窗口大小
    setFixedSize(width, height);
    // 设置文本渲染器窗口大小
    TextRenderer::getInstance()->setWindowSize(width, height);
    TextRenderer::getInstance()->setGlobalFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    TextRenderer::getInstance()->setHoldDuration(1.0f);  // 停留1.2秒
    TextRenderer::getInstance()->setGravity(600.0f);     // 更快的下坠速度
    TextRenderer::getInstance()->setDampFactor(0.85f);   // 更强的弹性效果

    this->setWindowFlag(Qt::FramelessWindowHint); // 设置无边框窗口
    this->setWindowFlag(Qt::WindowStaysOnTopHint); // 设置窗口始终在顶部
    this->setWindowFlag(Qt::Tool); // 隐藏应用程序图标
    this->setAttribute(Qt::WA_TranslucentBackground); // 设置窗口背景透明

    // 帧率控制初始化
    frameTimer = new QTimer(this);
    connect(frameTimer, &QTimer::timeout, [&]() {
        update();
    });
    frameTimer->start(static_cast<int>((1.0 / frameRate) * 1000)); // 使用成员变量计算间隔

#ifdef Q_OS_WIN
    // 保存窗口句柄
    hwnd = reinterpret_cast<HWND>(this->winId());
#endif

    // 启用鼠标跟踪，不启用的话鼠标按下才会回调mouseMoveEvent函数
    this->setMouseTracking(true);

    // 连接一些必要的信号与槽
#ifndef EMBEDDED_LINUX      // 如果不是嵌入式Linux系统(注意如果你的嵌入式Linux平台启用了桌面系统，那么可能需要去掉这个条件宏)
    connect(contextMenu, &Menu::closeMainWindow, this, &GLCore::closeGL);   // 关闭窗口信号
#endif

#ifdef EMBEDDED_LINUX
    AppCore::getInstance();

    pttButton = new PttButton(this);
    pttButton->updateLayout(width, height);

    connect(pttButton, &PttButton::pressedForPtt, this, []() {
        AppCore::getInstance()->startPttRecording();
    });
    connect(pttButton, &PttButton::releasedForPtt, this, []() {
        AppCore::getInstance()->stopPttRecording();
    });
#endif

    // 注册当前实例到中介类
    AppContext::RegisterGLCore(this);
}


GLCore::~GLCore()
{
    // 注销实例
    AppContext::UnregisterGLCore();

    // 释放TextRender单例
    TextRenderer::releaseInstance();

    // 释放Live2D 单例
    LAppDelegate::ReleaseInstance();
}

// 帧率设置
void GLCore::setFrameRate(double fps)
{
    if (qFuzzyCompare(fps, frameRate))  // 避免无意义更新
        return;

    if (fps <= 0.0) {
        qWarning() << "Invalid frame rate:" << fps << "using default 60.0";
        fps = 60.0;
    }

    frameRate = fps;
    frameTimer->setInterval(static_cast<int>((1.0 / frameRate) * 1000));
}

// 获取当前帧率
double GLCore::getFrameRate() const
{
    return frameRate;
}

QMap<QString, double> GLCore::getFrameRateMap()
{
    return frameRateMap;
}

QStringList GLCore::getFrameRateList()
{
    // 将 frameRateMap中的String部分转换为 QStringList
    QStringList frameRateList;
    for (auto it = frameRateMap.begin(); it != frameRateMap.end(); ++it) {
        frameRateList.append(it.key());
    }
    // 将frameRateList的数字字符从小到大排序
    std::sort(frameRateList.begin(), frameRateList.end(), [](const QString& a, const QString& b) {
        return a.toDouble() < b.toDouble();
    });
    // 将60放在第一个位置
    std::swap(frameRateList[0], frameRateList[frameRateList.indexOf("60")]);
    return frameRateList;
}

/**
 * 关闭窗口
 */
void GLCore::closeGL()
{
    this->close();
}

/**
 * 主要是为setWindowFlag(Qt::Tool)这段代码擦屁股。
 * 在 Qt 中，程序的退出通常依赖于主事件循环（QApplication的事件循环）的退出。当主窗口关闭时，通常会触发QApplication的lastWindowClosed信号，从而退出事件循环，导致程序退出。
    然而，当你将窗口设置为工具窗口（Qt::Tool）时，这个窗口可能不会被视为应用程序的“主窗口”，因此关闭它可能不会触发lastWindowClosed信号，导致程序不会正常退出。
 * @param event
 */
void GLCore::closeEvent(QCloseEvent* event)
{
    QApplication::quit();  // 显式退出事件循环
    event->accept();  // 确保关闭事件被接受
}

#ifdef Q_OS_WIN
void GLCore::setWindowTransparentForMouse(const bool transparent) const {
    if (!hwnd) return;

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (transparent) {
        // 启用鼠标穿透
        exStyle |= WS_EX_TRANSPARENT;
        exStyle |= WS_EX_LAYERED;
    } else {
        // 禁用鼠标穿透
        exStyle &= ~WS_EX_TRANSPARENT;
        exStyle &= ~WS_EX_LAYERED;
    }

    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    // 刷新窗口
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}
#endif

void GLCore::mouseMoveEvent(QMouseEvent* event)
{
    const float x = static_cast<float>(event->position().x());
    const float y = static_cast<float>(event->position().y());
    LAppDelegate::GetInstance()->GetView()->OnTouchesMoved(x, y);   // 将当前鼠标位置传递给LAppDelegate

#if !defined(EMBEDDED_LINUX)
    if (isLeftPressed) {    // 鼠标左键按下
        const QPoint newPos = event->globalPos() - currentPos;
        this->move(newPos);
    }
#endif
}

void GLCore::mousePressEvent(QMouseEvent* event)
{
    const float x = static_cast<float>(event->position().x());
    const float y = static_cast<float>(event->position().y());
    // 检测是否在模型上
    bool onModel = false;
    if (LAppDelegate::GetInstance() && LAppDelegate::GetInstance()->GetView()) {
        onModel = LAppDelegate::GetInstance()->GetView()->IsModelHit(x, y);
    }

    if (event->button() == Qt::LeftButton) {
        LAppDelegate::GetInstance()->GetView()->OnTouchesBegan(x, y);
        this->currentPos = event->globalPos() - this->frameGeometry().topLeft();
        if (onModel) {
            // 窗口拖动
            this->isLeftPressed = true;
#ifdef Q_OS_WIN
            // 确保窗口不穿透
            setWindowTransparentForMouse(false);
#endif
        } else {
            // 透明区域：透传(只有WIndows完美实现了，Linux由于平台差异，只是简单实现，并没有完美透传功能)
#ifdef Q_OS_WIN
            // 设置窗口为鼠标穿透
            setWindowTransparentForMouse(true);

            // 发送鼠标按下事件到底层窗口
            POINT pt = { event->globalPos().x(), event->globalPos().y() };
            HWND hWndBelow = WindowFromPoint(pt);
            if (hWndBelow && hWndBelow != hwnd) {
                // 转换坐标
                ScreenToClient(hWndBelow, &pt);

                // 发送鼠标按下消息
                PostMessage(hWndBelow, WM_LBUTTONDOWN,
                           MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
                PostMessage(hWndBelow, WM_LBUTTONUP,
                           0, MAKELPARAM(pt.x, pt.y));
            }

            // 恢复窗口不穿透状态（下一次鼠标移动时会重新检测）
            QTimer::singleShot(100, this, [this]() {
                setWindowTransparentForMouse(false);
            });
#endif
            this->isLeftPressed = false;
        }
    }
    // TODO: 右键菜单等
    if (event->button() == Qt::RightButton) {
        // 在鼠标右键点击的位置创建菜单，显示自定义右键菜单
        if (onModel) {
#if !defined(EMBEDDED_LINUX)
            contextMenu->showMenu(event->globalPos());
#endif
            this->isRightPressed = true;
        }
        else {
#ifdef Q_OS_WIN
            // 设置窗口为鼠标穿透
            setWindowTransparentForMouse(true);

            // 发送鼠标按下事件到底层窗口
            POINT pt = { event->globalPos().x(), event->globalPos().y() };
            HWND hWndBelow = WindowFromPoint(pt);
            if (hWndBelow && hWndBelow != hwnd) {
                // 转换坐标
                ScreenToClient(hWndBelow, &pt);

                // 发送鼠标按下消息
                PostMessage(hWndBelow, WM_LBUTTONDOWN,
                           MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
                PostMessage(hWndBelow, WM_LBUTTONUP,
                           0, MAKELPARAM(pt.x, pt.y));
            }

            // 恢复窗口不穿透状态（下一次鼠标移动时会重新检测）
            QTimer::singleShot(100, this, [this]() {
                setWindowTransparentForMouse(false);
            });
#endif
            this->isRightPressed = false;
        }
    }
}

void GLCore::mouseReleaseEvent(QMouseEvent* event)
{
    const float x = static_cast<float>(event->position().x());
    const float y = static_cast<float>(event->position().y());
    LAppDelegate::GetInstance()->GetView()->OnTouchesEnded(x, y);

    if (event->button() == Qt::LeftButton) {
        isLeftPressed = false;
    }
    if (event->button() == Qt::RightButton) {
        isRightPressed = false;
    }
}

void GLCore::initializeGL()
{
    // 注入渲染后端抽象层，由GLCore(OpenGL)创建GLRenderContext并交给LAppDelegate管理
    if (!LAppDelegate::GetInstance()->GetRenderContext()) {
        LAppDelegate::GetInstance()->SetRenderContext(new GLRenderContext());
    }

    #ifndef EMBEDDED_LINUX
    // 初始化GLEW 必须在任何Live2D渲染操作之前调用
    // Live2D预编译的libFramework.a使用GLEW函数指针（如glGenFramebuffers等），
    // 未调用glewInit()会导致空指针解引用SIGSEGV
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        qFatal("Failed to initialize GLEW");
        return;
    }
    #endif

    // 注册窗口大小变更回调 模型加载后通过此回调通知GLCore调整窗口
    LAppDelegate::GetInstance()->SetWindowResizeCallback([this](int w, int h) {
        setWindowSize(w, h);
    });
    // LAppDelegate::GetInstance()->Initialize(this);  // 原(QWidget*)
    LAppDelegate::GetInstance()->Initialize(this->width(), this->height()); // 解耦
}

void GLCore::paintGL()
{
    // Live2D Model画面渲染
    LAppDelegate::GetInstance()->update();
    // 渲染文本
    TextRenderer::getInstance()->update();
    TextRenderer::getInstance()->render();
#ifdef YOSUGA_DEBUG
    fpsOverlay.tick();
    fpsOverlay.draw(width(), height());
#endif
}

void GLCore::resizeGL(const int w, const int h)
{
    // 设置文本渲染器窗口大小
    TextRenderer::getInstance()->setWindowSize(w, h);

    LAppDelegate::GetInstance()->resize(w, h);

#ifdef EMBEDDED_LINUX
    pttButton->updateLayout(w, h);
#endif
}

// 设置窗口大小，并触发 resizeGL 事件
void GLCore::setWindowSize(const int w, const int h)
{
    // 检查是否需要更新，避免重复调用
    if (this->width() == w && this->height() == h) {
        return;
    }
    // 调用 QWidget::resize 或 setFixedSize 来改变窗口的实际尺寸
    setFixedSize(w, h);
    // 调用 setFixedSize 会自动触发 QOpenGLWidget 的 resizeEvent，
    // 进而调用 resizeGL(w, h)，无需手动调用 resizeGL
}
