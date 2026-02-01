//
// Created by misaki on 2026/2/1.
//
/**
 * 屏幕截图与系统信息获取工具类
 */

#pragma once
#include <QString>
#include <QSize>
#include <QScreen>
#include <QPixmap>
#include <QBuffer>
#include <QGuiApplication>
#include <QWindow>
#include <QCursor>
#include <QSysInfo>
class ScreenHelper
{
public:
    // 系统信息struct
    struct SystemInfo {
        QString osType;         // 例如: "windows", "linux", "macos"
        QString osVersion;      // 例如: "Windows 11 (10.0)", "Ubuntu 22.04"
        QString displayServer;  // 例如: "windows", "cocoa", "xcb" (X11), "wayland"
        bool isWayland;         // 专门标记是否为 Wayland
    };

    // 截图结果struct
    struct ScreenshotResult {
        bool success;           // 是否成功
        QString base64Data;     // 图片的Base64字符串 (PNG格式)
        int width;              // 图片宽度
        int height;             // 图片高度
        QString screenName;     // 屏幕名称
        QString errorMsg;       // 如果失败，返回错误信息
    };
public:
    /**
     * @brief 获取当前焦点屏幕的全屏截图并转换为Base64 \n
     * 判定逻辑：优先取有焦点的窗口所在屏幕，若无，取鼠标所在屏幕
     */
    static ScreenshotResult captureFocusedScreen();

    /**
     * @brief 获取当前操作系统和显示服务信息
     */
    static SystemInfo getSystemInfo();

private:
    // 私有构造，禁止实例化
    ScreenHelper() = default;
};