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
#include "serialportmanager.h"

class DeviceTcpServer;
class DeviceWebSocketServer;

class AppCore final : public QObject {
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
    void tryToInit() { return; };
};