//
// Created by Yosuga on 2026/4/25.
//

/**
 * 设备数据处理模块 — 统一路由器
 *
 * 管理三种设备连接通道:
 *   - TCP (RK3566 等通过 TCP 接入)
 *   - WebSocket (ESP32 等通过 WebSocket 接入)
 *   - 串口 (STM32 等通过串口接入)
 *
 * 数据流向:
 *   Device →(TCP/WS/Serial)→ DeviceDataHandle → NetworkDO → WebSocket → YosugaServer
 *   YosugaServer → WebSocket → NetworkDO → DeviceDataHandle →(TCP/WS/Serial)→ Device
 */

#pragma once
#include <QObject>
#include <QMutex>
#include <QScopedPointer>
#include <QHash>
#include <functional>
#include "DeviceDataTransferObject.h"
#include "serialportmanager.h"

class DeviceDataHandle final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DeviceDataHandle)

private:
    explicit DeviceDataHandle(QObject *parent = nullptr);
    static QScopedPointer<DeviceDataHandle> m_instance;
    static QMutex m_mutex;

public:
    static DeviceDataHandle *getInstance();
    static void destroy();
    ~DeviceDataHandle() override;

    // 注册设备到路由表（由各 Server/Client 在设备握手完成后调用）
    void registerDevice(const QString &deviceId, const QString &deviceType, QObject *connection);

    // 移除设备
    void unregisterDevice(const QString &deviceId);

    // 向设备发送数据（自动选择正确的通道）
    void sendToDevice(const QString &deviceId, const QString &type, const QJsonObject &data);

    // 获取设备连接类型
    [[nodiscard]] QString deviceConnectionType(const QString &deviceId) const;

public slots:
    // 收到来自 YosugaServer 的设备命令（通过 device_command 信号）
    void onDeviceCommandReceived(const DeviceDataTransferObject &packet);

    // 收到来自 TCP 设备的 JSON 数据
    void onTcpDeviceData(const QString &deviceId, const QString &type, const QJsonObject &data);

    // 收到来自 WebSocket 设备的 JSON 数据
    void onWsDeviceData(const QString &deviceId, const QString &type, const QJsonObject &data);

    // 收到来自串口设备的 JSON 数据
    void onSerialDeviceData(const QString &deviceId, const QString &type, const QJsonObject &data);

private:
    // 转发设备数据到 YosugaServer
    void forwardToServer(const QString &deviceId, const QString &type, const QJsonObject &data);

    struct DeviceEntry {
        QString deviceId;
        QString deviceType;   // "tcp", "websocket", "serial"
        QObject *connection;  // DeviceTcpServer / DeviceWebSocketServer / SerialPortClient
    };
    QHash<QString, DeviceEntry> m_devices;
};
