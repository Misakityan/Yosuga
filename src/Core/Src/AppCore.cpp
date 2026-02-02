//
// Created by misaki on 2026/1/24.
//

#include "AppCore.h"
#include <QDebug>

#include "AudioDataHandle.h"
#include "AutoAgentHandle.h"
#include "ScreenShotReqDataHandle.h"

#include "AudioInput.h"
#include "NetWorkDO.h"
#include "websocketmanager.h"
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
        this, &AppCore::onRecordingFinished_Byte);      // 录音完成信号
}

AppCore::~AppCore()
{
    // 析构业务解析单例
    ScreenShotReqDataHandle::destroy();
    AutoAgentHandle::destroy();     // 显式销毁
    AudioDataHandle::destroy();


    qDebug() << "AppCore destroyed";
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

