//
// Created by misaki on 2025/12/29.
//
#include "NetWorkDO.h"

#include <QDebug>
#include <QMutexLocker>

// 初始化静态成员
QScopedPointer<NetworkDO> NetworkDO::m_instance;
QMutex NetworkDO::m_mutex;

// 单例实现 (QScopedPointer + Mutex)
NetworkDO* NetworkDO::getInstance()
{
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            // 使用 reset 创建实例，因为构造函数是私有的
            m_instance.reset(new NetworkDO());
        }
    }
    return m_instance.data();
}

void NetworkDO::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset(); // 这会触发析构函数
    }
}

NetworkDO::NetworkDO(QObject *parent) : QObject(parent)
{
    qDebug() << "NetworkDO initialized";
}

NetworkDO::~NetworkDO()
{
    qDebug() << "NetworkDO destroyed";
}

// 业务逻辑实现
void NetworkDO::registerSender(SenderFunc sender)
{
    QMutexLocker locker(&m_mutex); // 加个小锁，简单保护一下赋值
    m_sender = std::move(sender);
}

void NetworkDO::sendPacket(const DataTransferObjectBase &packet)
{
    // 检查发送器是否已注入
    if (!m_sender) {
        emit errorOccurred("Sender not registered! Call registerSender() first.");
        return;
    }
    // 依赖注入 + 多态实现完美解耦
    m_sender(packet.type(), packet.toJson());
}

// 接受并没有完全解耦
void NetworkDO::onDataReceived(const QString& type, const QJsonObject& data)
{
    // 根据类型分发数据包
    // 为什么分发做在这里，而不是统一数据再去分发，如果不在这里做分发通知，分开发信号，而使用统一的信号
    // 如果有多个观察者，让观察者自动识别数据包，这会导致信号广播，容易引起性能问题(因为这里依赖的是Qt的信号与槽机制)
    // TODO: 考虑在此处使用工厂模式，根据type内容快速创建对应的对象
    if (type == "audio_data") {
        emit audioPacketReceived(AudioDataTransferObject::fromJson(data));      // 构造并发送音频对象
    }
    else if (type == "auto_agent") {
        emit autoAgentPacketReceived(AutoAgentDataObject::fromJson(data));
    }
    else if (type == "screenshot_data") {
        emit screenShotPacketReceived(ScreenShotDataTransferObject::fromJson(data));
    }
    else {
        qWarning() << "[NetworkDO] Received unknown type:" << type;
    }
}
