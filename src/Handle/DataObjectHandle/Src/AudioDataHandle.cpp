//
// Created by misaki on 2026/1/30.
//
#include "AudioDataHandle.h"
#include "NetWorkDO.h"
#include "AudioOutput.h"
// 初始化静态成员
QScopedPointer<AudioDataHandle> AudioDataHandle::m_instance;
QMutex AudioDataHandle::m_mutex;

// 单例实现 (QScopedPointer + Mutex)
AudioDataHandle* AudioDataHandle::getInstance()
{
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            // 使用 reset 创建实例，因为构造函数是私有的
            m_instance.reset(new AudioDataHandle());
        }
    }
    return m_instance.data();
}

void AudioDataHandle::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset(); // 这会触发析构函数
    }
}


AudioDataHandle::AudioDataHandle(QObject *parent) : QObject(parent)
{
    connect(NetworkDO::getInstance(), &NetworkDO::audioPacketReceived, this, &AudioDataHandle::onAudioPacketReceived);
}

AudioDataHandle::~AudioDataHandle()
{
    qDebug() << "AutoAgentHandle destroyed";
}

void AudioDataHandle::onAudioPacketReceived(const AudioDataTransferObject &packet) {
    // 管理并调用AudioOutput播放流式wav音频
    if (packet.isEnd()) {   // 如果是结束包(空包)
        AudioOutput::getInstance()->stopStream();   // 停止播放
        return;
    }
    if (packet.isStart()) { // 如果是开始包(单wav 44字节头)
        AudioOutput::getInstance()->startStream(packet.sampleRate(), packet.channelCount(), packet.bitDepth());;  // 播放开始
        return;
    }
    // 否则加入播放队列即可
    AudioOutput::getInstance()->pushStreamData(packet.audioData());
}

