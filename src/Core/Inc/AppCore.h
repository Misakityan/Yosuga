//
// Created by misaki on 2026/1/24.
//

#pragma once

/**
 *  客户端业务核心
 *  1. 处理来自服务端的数据，分发并执行
 *  2. 完成非阻塞的事件循环处理，构建业务状态机
 */
#include <QMutex>
#include <QObject>
#include <QHash>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_LINUX
#include <QAbstractNativeEventFilter>
#endif
#include "serialportmanager.h"

class DeviceTcpServer;
class DeviceWebSocketServer;

class AppCore final : public QObject
#ifdef Q_OS_LINUX
    , public QAbstractNativeEventFilter
#endif
{
    Q_OBJECT
    Q_DISABLE_COPY(AppCore)

    private:
    explicit AppCore(QObject *parent = nullptr);

    static QScopedPointer<AppCore> m_instance;
    static QMutex m_mutex;

    DeviceTcpServer *m_deviceTcpServer = nullptr;
    DeviceWebSocketServer *m_deviceWsServer = nullptr;

private slots:
    void onRecordingFinished_Byte(const QByteArray &wavData);

public:
    static AppCore *getInstance();
    static void destroy();

    ~AppCore() override;

    void registerEmbeddedDevice(const QString &deviceId, SerialPortClient *client);
    void unregisterEmbeddedDevice(const QString &deviceId);

public:
    void SingleExchange();
    void tryToInit() { return; }

    // PTT 按住说话
    void startPttRecording();
    void stopPttRecording();

    void setupGlobalHotkey();
    void cleanupGlobalHotkey();

#if defined(Q_OS_WIN)
private:
    static LRESULT CALLBACK lowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);
    HHOOK m_keyboardHook = nullptr;
    bool m_isPttDown = false;
    UINT m_hotkeyVKey = VK_OEM_3;

#elif defined(Q_OS_LINUX)
private:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
    void onDebounceStop();
    bool m_isPttDown = false;
    int m_hotkeyCode = 0;
    class QTimer *m_pttDebounce = nullptr;

#elif defined(Q_OS_MACOS)
private:
    void *m_eventTap = nullptr;
    void *m_runLoopSource = nullptr;
    bool m_isPttDown = false;
    int m_hotkeyCode = 50;
#endif
};