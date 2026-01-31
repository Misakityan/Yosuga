//
// Created by misaki on 2026/1/30.
//

#pragma once

#include <QObject>
#include <QMutex>

#include "AudioDataTransferObject.h"

class AudioDataHandle final : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(AudioDataHandle)     // 禁用拷贝
    private:
    /**
     * 构造函数私有化
     * @param parent
     */
    explicit AudioDataHandle(QObject *parent = nullptr);        // 并不将本模块挂在对象树当中，因为本模块为单例类，内存自行管理

    static QScopedPointer<AudioDataHandle> m_instance;       // 单例类
    static QMutex m_mutex;
private slots:
    // 业务接收槽函数，当获取到音频数据包时，进行解析并播放
    void onAudioPacketReceived(const AudioDataTransferObject& packet);
public:
    // 单例访问点
    static AudioDataHandle *getInstance();
    // 显式销毁
    static void destroy();

    ~AudioDataHandle() override;
};
