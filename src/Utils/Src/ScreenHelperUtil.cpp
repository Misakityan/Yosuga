//
// Created by misaki on 2026/2/1.
//
#include <QDebug>
#include "ScreenHelperUtil.hpp"
ScreenHelper::ScreenshotResult ScreenHelper::captureFocusedScreen()
{
    ScreenHelper::ScreenshotResult result;
    result.success = false;
    // 获取目标屏幕
    QScreen *targetScreen = nullptr;

    // 首先尝试获取当前应用程序拥有焦点的窗口所在的屏幕
    QWindow *focusWindow = QGuiApplication::focusWindow();
    if (focusWindow) {
        targetScreen = focusWindow->screen();
    }
    // 如果没有窗口焦点或者窗口还没显示，获取鼠标光标所在的屏幕
    if (!targetScreen) {
        targetScreen = QGuiApplication::screenAt(QCursor::pos());
    }

    // 如果以上都失败，回退到主屏幕
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }

    if (!targetScreen) {
        result.errorMsg = "Critical Error: No detectible screen found.";
        return result;
    }

    // 获取屏幕基本信息
    result.screenName = targetScreen->name();

    // 执行截图
    // grabWindow(0) 表示截取整个屏幕
    // 注：在 Wayland 上，这可能需要系统权限或会弹出确认框，或者在某些安全策略下返回黑色图像
    QPixmap pixmap = targetScreen->grabWindow(0);

    if (pixmap.isNull()) {
        result.errorMsg = "Failed to grab screen content (Permission denied or System restriction).";
        return result;
    }

    result.width = pixmap.width();
    result.height = pixmap.height();

    // 转换为 Base64
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    // 保存为 PNG 格式，质量默认即可
    if (pixmap.save(&buffer, "PNG")) {
        result.base64Data = QString::fromLatin1(byteArray.toBase64());
        result.success = true;
    } else {
        result.errorMsg = "Failed to encode image to PNG buffer.";
    }

    return result;
}

ScreenHelper::SystemInfo ScreenHelper::getSystemInfo()
{
    ScreenHelper::SystemInfo info;
    // 获取操作系统类型
    info.osType = QSysInfo::productType();

    // 获取详细版本 (例如 Windows 10/11, Ubuntu 20.04)
    // prettyProductName() 通常能区分 Win10 和 Win11
    info.osVersion = QSysInfo::prettyProductName();

    // 获取显示服务器类型 (Platform Plugin)
    // 这里的返回值通常是 QPA 插件的名字
    // Windows -> "windows"
    // macOS -> "cocoa"
    // Linux X11 -> "xcb"
    // Linux Wayland -> "wayland"
    QString platformName = QGuiApplication::platformName();
    info.displayServer = platformName;

    // 专门判断 Wayland
    info.isWayland = (platformName == "wayland");
    // 针对 Linux 做更细致的显示名称优化
    if (platformName == "xcb") {
        info.displayServer = "X11 (xcb)";
    } else if (platformName == "wayland") {
        info.displayServer = "Wayland";
    }
    return info;
}