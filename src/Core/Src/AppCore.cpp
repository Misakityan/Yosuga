//
// Created by misaki on 2026/1/24.
//

#include "AppCore.h"
#include <QDebug>

#include "AudioDataHandle.h"
#include "AutoAgentHandle.h"
#include "ScreenShotReqDataHandle.h"
#include "DeviceDataHandle.h"

#include "AudioInput.h"
#include "NetWorkDO.h"
#include "websocketmanager.h"
#include "DeviceTcpServer.h"
#include "DeviceWebSocketServer.h"
// 初始化静态成员
QScopedPointer<AppCore> AppCore::m_instance;
QMutex AppCore::m_mutex;

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
    AutoAgentHandle::getInstance();
    ScreenShotReqDataHandle::getInstance();
    // 注入发送接口
    NetworkDO::getInstance()->registerSender([](const QString& type, const QJsonObject& data){
        WebSocketClient::getInstance()->sendJson(type, data);
    });
    // TODO Test
    AudioInput::getInstance()->setAudioPath(QDir::currentPath(), "/temp.wav");
    // 连接必要的信号
    connect(AudioInput::getInstance(), &AudioInput::recordingFinished_Byte,
        this, &AppCore::onRecordingFinished_Byte);
}

AppCore::~AppCore()
{
    if (m_deviceTcpServer) m_deviceTcpServer->stop();
    if (m_deviceWsServer) m_deviceWsServer->stop();

    ScreenShotReqDataHandle::destroy();
    AutoAgentHandle::destroy();
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

