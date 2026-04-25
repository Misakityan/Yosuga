//
// Created by Yosuga on 2026/4/25.
//

#include "DeviceDataHandle.h"
#include "NetWorkDO.h"
#include <QDebug>
#include <QMetaMethod>

QScopedPointer<DeviceDataHandle> DeviceDataHandle::m_instance;
QMutex DeviceDataHandle::m_mutex;

DeviceDataHandle *DeviceDataHandle::getInstance()
{
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            m_instance.reset(new DeviceDataHandle());
        }
    }
    return m_instance.data();
}

void DeviceDataHandle::destroy()
{
    QMutexLocker locker(&m_mutex);
    m_instance.reset();
}

DeviceDataHandle::DeviceDataHandle(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QJsonObject>("QJsonObject");

    connect(NetworkDO::getInstance(), &NetworkDO::deviceCommandReceived,
            this, &DeviceDataHandle::onDeviceCommandReceived);
}

DeviceDataHandle::~DeviceDataHandle()
{
    qDebug() << "[DeviceDataHandle] destroyed";
}

void DeviceDataHandle::registerDevice(const QString &deviceId, const QString &deviceType, QObject *connection)
{
    DeviceEntry entry;
    entry.deviceId = deviceId;
    entry.deviceType = deviceType;
    entry.connection = connection;
    m_devices.insert(deviceId, entry);

    qDebug() << "[DeviceDataHandle] 设备已注册:" << deviceId << "类型:" << deviceType;
}

void DeviceDataHandle::unregisterDevice(const QString &deviceId)
{
    m_devices.remove(deviceId);
    qDebug() << "[DeviceDataHandle] 设备已移除:" << deviceId;
}

void DeviceDataHandle::sendToDevice(const QString &deviceId, const QString &type, const QJsonObject &data)
{
    DeviceEntry entry = m_devices.value(deviceId);
    if (entry.connection == nullptr) {
        qWarning() << "[DeviceDataHandle] 未知设备:" << deviceId;
        return;
    }

    // 通过 QMetaObject::invokeMethod 动态调用对应 Server 的 sendToDevice
    bool ok = QMetaObject::invokeMethod(
        entry.connection,
        "sendToDevice",
        Qt::QueuedConnection,
        Q_ARG(QString, deviceId),
        Q_ARG(QString, type),
        Q_ARG(QJsonObject, data)
    );

    if (!ok) {
        // 兜底：如果是串口设备，直接调用 SerialPortClient::sendJson
        auto *serialClient = qobject_cast<SerialPortClient*>(entry.connection);
        if (serialClient) {
            serialClient->sendJson(type, data);
            ok = true;
        }
    }

    if (!ok) {
        qWarning() << "[DeviceDataHandle] 发送失败到设备:" << deviceId;
    }
}

QString DeviceDataHandle::deviceConnectionType(const QString &deviceId) const
{
    return m_devices.value(deviceId).deviceType;
}

void DeviceDataHandle::onDeviceCommandReceived(const DeviceDataTransferObject &packet)
{
    const QString deviceId = packet.deviceId();
    if (deviceId.isEmpty()) {
        qWarning() << "[DeviceDataHandle] device_command 缺少 device_id";
        return;
    }

    // 提取 RPC 调用字符串
    QJsonObject payload = packet.payload();
    QString rpcCall;
    if (payload.contains("rpc_call")) {
        rpcCall = payload.value("rpc_call").toString();
    } else {
        rpcCall = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }

    QJsonObject forwardPayload;
    forwardPayload["rpc_call"] = rpcCall;
    sendToDevice(deviceId, "rpc_call", forwardPayload);
    qDebug() << "[DeviceDataHandle] 已转发命令到设备:" << deviceId;
}

void DeviceDataHandle::onTcpDeviceData(const QString &deviceId, const QString &type, const QJsonObject &data)
{
    forwardToServer(deviceId, type, data);
}

void DeviceDataHandle::onWsDeviceData(const QString &deviceId, const QString &type, const QJsonObject &data)
{
    forwardToServer(deviceId, type, data);
}

void DeviceDataHandle::onSerialDeviceData(const QString &deviceId, const QString &type, const QJsonObject &data)
{
    forwardToServer(deviceId, type, data);
}

void DeviceDataHandle::forwardToServer(const QString &deviceId, const QString &type, const QJsonObject &data)
{
    // 所有设备数据统一封装为 DeviceDataTransferObject 发往 YosugaServer
    DeviceDataTransferObject packet(type, deviceId, data);
    NetworkDO::getInstance()->sendPacket(packet);
    qDebug() << "[DeviceDataHandle] 设备数据已转发到服务端:" << deviceId << type;
}
