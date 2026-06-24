//
// Created by misaki on 2026/1/24.
//

#include "AppCore.h"
#include <QDebug>
#include <QApplication>
#include <QTimer>

#include "AudioDataHandle.h"
#if !defined(EMBEDDED_LINUX)
#include "AutoAgentHandle.h"
#include "ScreenShotReqDataHandle.h"
#endif
#include "DeviceDataHandle.h"

#include "AudioInput.h"
#include "NetWorkDO.h"
#include "websocketmanager.h"
#include "DeviceTcpServer.h"
#include "DeviceWebSocketServer.h"

#if defined(Q_OS_LINUX) && !defined(EMBEDDED_LINUX)
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#endif

#ifdef Q_OS_MACOS
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

// 初始化静态成员
QScopedPointer<AppCore> AppCore::m_instance;
QMutex AppCore::m_mutex;

#ifdef Q_OS_WIN
static AppCore *g_hotkeyInstance = nullptr;
#endif

// 单例实现 (QScopedPointer + Mutex)
AppCore* AppCore::getInstance()
{
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            // 使用 reset 创建实例，因为构造函数是私有的
            m_instance.reset(new AppCore());
        }
    }
    return m_instance.data();
}

void AppCore::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset(); // 这会触发析构函数
    }
}

AppCore::AppCore(QObject *parent) : QObject(parent)
{
    DeviceDataHandle *deviceHandle = DeviceDataHandle::getInstance();

    // 启动嵌入式设备 TCP 服务器
    m_deviceTcpServer = new DeviceTcpServer(10001, this);
    connect(m_deviceTcpServer, &DeviceTcpServer::deviceConnected,
            deviceHandle, [=](const QString &deviceId, const QString &) {
                deviceHandle->registerDevice(deviceId, "tcp", m_deviceTcpServer);
            });
    connect(m_deviceTcpServer, &DeviceTcpServer::deviceDisconnected,
            deviceHandle, &DeviceDataHandle::unregisterDevice);
    connect(m_deviceTcpServer, &DeviceTcpServer::jsonReceived,
            deviceHandle, &DeviceDataHandle::onTcpDeviceData);
    m_deviceTcpServer->start();

    // 启动嵌入式设备 WebSocket 服务器
    m_deviceWsServer = new DeviceWebSocketServer(10002, this);
    connect(m_deviceWsServer, &DeviceWebSocketServer::deviceConnected,
            deviceHandle, [=](const QString &deviceId, const QString &) {
                deviceHandle->registerDevice(deviceId, "websocket", m_deviceWsServer);
            });
    connect(m_deviceWsServer, &DeviceWebSocketServer::deviceDisconnected,
            deviceHandle, &DeviceDataHandle::unregisterDevice);
    connect(m_deviceWsServer, &DeviceWebSocketServer::jsonReceived,
            deviceHandle, &DeviceDataHandle::onWsDeviceData);
    m_deviceWsServer->start();

    // 初始化业务解析单例
    AudioDataHandle::getInstance();
#if !defined(EMBEDDED_LINUX)
    AutoAgentHandle::getInstance();
    ScreenShotReqDataHandle::getInstance();
#endif
    // 注入发送接口
    NetworkDO::getInstance()->registerSender([](const QString& type, const QJsonObject& data){
        WebSocketClient::getInstance()->sendJson(type, data);
    });
#ifdef EMBEDDED_LINUX
    // 嵌入式平台无Settings UI，需手动绑定WebSocket接收链路
    connect(WebSocketClient::getInstance(), &WebSocketClient::jsonReceived,
            NetworkDO::getInstance(), &NetworkDO::onDataReceived);
    // 自动配置并连接服务端（优先读取环境变量 YOSUGA_SERVER_URL）
    {
        QString serverUrl = qEnvironmentVariable("YOSUGA_SERVER_URL", "ws://192.168.1.18:8765");
        auto *client = WebSocketClient::getInstance();
        client->setConfiguration(QUrl(serverUrl));
        client->setAutoReconnect(true);
        client->connectToServer();
        qDebug() << "[AppCore] Embedded: connecting to server:" << serverUrl;
    }
#endif
#ifdef EMBEDDED_LINUX
    AudioInput::getInstance()->setAudioSettings(16000, 2);
#endif
    AudioInput::getInstance()->setAudioPath(QDir::currentPath(), "/temp.wav");
    // 连接必要的信号
    connect(AudioInput::getInstance(), &AudioInput::recordingFinished_Byte,
        this, &AppCore::onRecordingFinished_Byte);
#if !defined(EMBEDDED_LINUX)
    setupGlobalHotkey();
#endif

}

AppCore::~AppCore()
{
#if !defined(EMBEDDED_LINUX)
    cleanupGlobalHotkey();
#endif

    if (m_deviceTcpServer) m_deviceTcpServer->stop();
    if (m_deviceWsServer) m_deviceWsServer->stop();

#if !defined(EMBEDDED_LINUX)
    ScreenShotReqDataHandle::destroy();
    AutoAgentHandle::destroy();
#endif
    AudioDataHandle::destroy();
    DeviceDataHandle::destroy();

    qDebug() << "AppCore destroyed";
}

void AppCore::registerEmbeddedDevice(const QString &deviceId, SerialPortClient *client)
{
    DeviceDataHandle::getInstance()->registerDevice(deviceId, QStringLiteral("serial"), client);
}

void AppCore::unregisterEmbeddedDevice(const QString &deviceId)
{
    DeviceDataHandle::getInstance()->unregisterDevice(deviceId);
}

void AppCore::SingleExchange() {
    // 开始录音，录音结束后会触发录音完成信号
    AudioInput::getInstance()->startAutoStopAudio(AudioInput::getInstance()->getSilenceThreshold(), 800);
}

void AppCore::onRecordingFinished_Byte(const QByteArray &wavData) {
    // 将录音数据发送给服务端
    AudioDataTransferObject packet;
    packet.setData("isStream", false).setData("data", wavData.toBase64().data());
    NetworkDO::getInstance()->sendPacket(packet);
}

void AppCore::startPttRecording() {
    qDebug() << "PTT recording started";
    AudioInput::getInstance()->startAudio();
}

void AppCore::stopPttRecording() {
    qDebug() << "PTT recording stopped";
    AudioInput::getInstance()->stopAudio();
}

// ===================== Windows =====================
#ifdef Q_OS_WIN
void AppCore::setupGlobalHotkey() {
    g_hotkeyInstance = this;
    HMODULE hMod = GetModuleHandle(nullptr);
    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, lowLevelKeyboardHook, hMod, 0);
    if (m_keyboardHook) {
        qDebug() << "[AppCore] 全局 PTT 热键钩子已安装 (键码:" << m_hotkeyVKey << ")";
    } else {
        qWarning() << "[AppCore] 全局 PTT 热键钩子安装失败:" << GetLastError();
    }
}

void AppCore::cleanupGlobalHotkey() {
    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
        qDebug() << "[AppCore] 全局 PTT 热键钩子已卸载";
    }
    g_hotkeyInstance = nullptr;
}

LRESULT CALLBACK AppCore::lowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_hotkeyInstance) {
        KBDLLHOOKSTRUCT *pKb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (pKb->vkCode == g_hotkeyInstance->m_hotkeyVKey) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                if (!g_hotkeyInstance->m_isPttDown) {
                    g_hotkeyInstance->m_isPttDown = true;
                    QMetaObject::invokeMethod(qApp, []() {
                        AppCore::getInstance()->startPttRecording();
                    }, Qt::QueuedConnection);
                }
                return 1;
            } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                if (g_hotkeyInstance->m_isPttDown) {
                    g_hotkeyInstance->m_isPttDown = false;
                    QMetaObject::invokeMethod(qApp, []() {
                        AppCore::getInstance()->stopPttRecording();
                    }, Qt::QueuedConnection);
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// ===================== Linux (X11) =====================
#elif defined(Q_OS_LINUX) && !defined(EMBEDDED_LINUX)
void AppCore::setupGlobalHotkey() {
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        qWarning() << "[AppCore] 无法打开 X11 Display，全局热键不可用";
        return;
    }

    // 尝试开启 detectable auto-repeat（不一定持久影响其他连接，不影响 debounce 方案）
    int orig = 0;
    XkbSetDetectableAutoRepeat(display, True, &orig);

    KeyCode keycode = XKeysymToKeycode(display, XK_grave);
    if (!keycode) {
        qWarning() << "[AppCore] 未找到波浪号键的键码";
        XCloseDisplay(display);
        return;
    }
    m_hotkeyCode = keycode;

    XGrabKey(display, keycode, AnyModifier, DefaultRootWindow(display),
             True, GrabModeAsync, GrabModeAsync);
    XCloseDisplay(display);

    // debounce 定时器：KeyRelease 后等待 50ms，若没有新的 KeyPress 再停
    m_pttDebounce = new QTimer(this);
    m_pttDebounce->setSingleShot(true);
    connect(m_pttDebounce, &QTimer::timeout, this, &AppCore::onDebounceStop);

    qApp->installNativeEventFilter(this);
    qDebug() << "[AppCore] Linux 全局 PTT 热键已注册 (键码:" << m_hotkeyCode << ")";
}

void AppCore::cleanupGlobalHotkey() {
    qApp->removeNativeEventFilter(this);
    if (m_pttDebounce) {
        m_pttDebounce->stop();
        delete m_pttDebounce;
        m_pttDebounce = nullptr;
    }
    Display *display = XOpenDisplay(nullptr);
    if (display) {
        XUngrabKey(display, m_hotkeyCode, AnyModifier, DefaultRootWindow(display));
        XCloseDisplay(display);
    }
    m_hotkeyCode = 0;
    qDebug() << "[AppCore] Linux 全局 PTT 热键已卸载";
}

void AppCore::onDebounceStop() {
    if (m_isPttDown) {
        m_isPttDown = false;
        stopPttRecording();
    }
}

bool AppCore::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
    if (eventType != "xcb_generic_event_t")
        return false;

    auto *data = static_cast<const uint8_t *>(message);
    uint8_t type = data[0] & 0x7f;

    if ((type == 2 || type == 3) && data[1] == static_cast<uint8_t>(m_hotkeyCode)) {
        if (type == 2) {                              // KeyPress
            m_pttDebounce->stop();                    // 取消待决的停止
            if (!m_isPttDown) {
                m_isPttDown = true;
                startPttRecording();
            }
        } else {                                        // KeyRelease
            // 不立即停，等待 50ms 确认没有连发 KeyPress
            if (m_isPttDown)
                m_pttDebounce->start(50);
        }
        if (result) *result = 1;
        return true;
    }
    return false;
}

// ===================== macOS (CGEventTap) =====================
#elif defined(Q_OS_MACOS)
static CGEventRef pttEventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    auto *core = static_cast<AppCore *>(refcon);

    if (type == kCGEventTapDisabledByTimeout) {
        if (core && core->m_eventTap)
            CGEventTapEnable(static_cast<CFMachPortRef>(core->m_eventTap), true);
        return event;
    }
    if (type != kCGEventKeyDown && type != kCGEventKeyUp)
        return event;

    CGKeyCode kc = static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
    if (kc != core->m_hotkeyCode)
        return event;

    if (type == kCGEventKeyDown && !core->m_isPttDown) {
        core->m_isPttDown = true;
        core->startPttRecording();
    } else if (type == kCGEventKeyUp && core->m_isPttDown) {
        core->m_isPttDown = false;
        core->stopPttRecording();
    }
    return nullptr;
}

void AppCore::setupGlobalHotkey() {
    CFMachPortRef tap = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp),
        &pttEventTapCallback,
        this);

    if (!tap) {
        qWarning() << "[AppCore] macOS PTT 热键创建失败 (需要辅助功能权限)";
        return;
    }

    m_eventTap = tap;
    CFRunLoopSourceRef src = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    m_runLoopSource = src;
    CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);
    qDebug() << "[AppCore] macOS 全局 PTT 热键已注册 (键码:" << m_hotkeyCode << ")";
}

void AppCore::cleanupGlobalHotkey() {
    if (m_eventTap) {
        auto *tap = static_cast<CFMachPortRef>(m_eventTap);
        CGEventTapEnable(tap, false);
        if (m_runLoopSource) {
            auto *src = static_cast<CFRunLoopSourceRef>(m_runLoopSource);
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
            CFRelease(src);
        }
        CFRelease(tap);
        m_eventTap = nullptr;
        m_runLoopSource = nullptr;
    }
    qDebug() << "[AppCore] macOS 全局 PTT 热键已卸载";
}
#endif

