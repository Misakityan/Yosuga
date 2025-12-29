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
    QMutexLocker locker(&m_mutex); // 简单保护一下赋值
    m_sender = std::move(sender);
}

void NetworkDO::sendAudioPacket(const AudioDataPacket& packet)
{
    // 检查发送器是否已注入
    if (!m_sender) {
        emit errorOccurred("Sender not registered! Call registerSender() first.");
        return;
    }

    // 封装数据 (DTO -> JSON)
    QJsonObject dataObj;
    dataObj["text"] = packet.text;
    // 音频转 Base64 字符串传输
    dataObj["audio"] = QString::fromLatin1(packet.audioData.toBase64());
    dataObj["sampleRate"] = packet.sampleRate;
    dataObj["channels"] = packet.channels;
    dataObj["duration"] = packet.duration;

    // 调用底层发送 (解耦)
    // "textAudio" 是与后端约定的协议类型
    m_sender("textAudio", dataObj);
}

void NetworkDO::sendControlPacket(const ControlDataPacket& packet)
{
    if (!m_sender) return;

    QJsonObject dataObj;
    dataObj["action"] = static_cast<int>(packet.action);
    dataObj["x"] = packet.x;
    dataObj["y"] = packet.y;

    m_sender("control", dataObj);
}

void NetworkDO::onDataReceived(const QString& type, const QJsonObject& data)
{
    // 根据类型分发
    if (type == "textAudio") {
        handleAudioMessage(data);
    }
    else if (type == "control") {
        // handleControlMessage(data);
    }
    else {
        qWarning() << "[NetworkDO] Received unknown type:" << type;
    }
}

void NetworkDO::handleAudioMessage(const QJsonObject& data)
{
    AudioDataPacket packet;

    // 解析基础字段 (JSON -> DTO)
    packet.text = data.value("text").toString();
    packet.sampleRate = data.value("sampleRate").toInt(16000);
    packet.channels = data.value("channels").toInt(1);
    // 注意类型转换，确保 long long 精度
    packet.duration = static_cast<qint64>(data.value("duration").toDouble());

    // 解析音频 (Base64 -> Binary)
    QString base64Audio = data.value("audio").toString();
    if (!base64Audio.isEmpty()) {
        packet.audioData = QByteArray::fromBase64(base64Audio.toLatin1());
    }

    // 通知上层业务
    emit audioPacketReceived(packet);
}