//
// Created by Administrator on 2025/1/17.
//
#pragma once

#include <QObject>
#include <QMediaPlayer>     // 音频播放模块
#include <QAudioOutput>     // QMediaPlayer 的音量控制组件
#include <QAudioSink>       // 音频输出组件， 用于原始数据播放
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QUrl>
#include <QBuffer>
#include <QAudioFormat>

/**
 * @brief 音频播放模块
 * @author Misaki
 * 单例类
 * 本模块重新基于Qt6重构     2026.1.31第三次重构
 * 实现的功能
 * 1. 流式wav音频播放
 * 2. 根据音频文件路径播放音频
 */

// Worker 类定义 (负责流式音频的底层处理) 注意：此类实例将完全运行在子线程中
class StreamAudioWorker : public QObject {
Q_OBJECT
public:
    explicit StreamAudioWorker(QObject* parent = nullptr) : QObject(parent) {}
    ~StreamAudioWorker() override;

public slots:
    // 初始化并启动音频设备
    void start(int sampleRate, int channelCount, int bitDepth);
    // 处理接收到的音频数据块
    void processChunk(const QByteArray& chunk);
    // 停止播放并清理资源
    void stop();

    signals:
        void errorOccurred(const QString& msg);
    void playbackFinished(); // 流播放结束（通常指队列空了）

private:
    QScopedPointer<QAudioSink> m_sink;
    QIODevice* m_ioDevice = nullptr; // 由 m_sink->start() 返回，不需要且不能手动 delete
    bool m_firstChunk = true;        // 标记是否是第一块数据（用于剥离WAV头）
};

// AudioOutput 主类 (单例，线程安全)
class AudioOutput : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(AudioOutput)

private:
    explicit AudioOutput(QObject *parent = nullptr);
    static QScopedPointer<AudioOutput> m_instance;
    static QMutex m_mutex;

public:
    static AudioOutput *getInstance();
    static void destroy(); // 显式销毁
    ~AudioOutput() override;

    //  通用控制接口
    // 停止所有播放 (文件和流)
    void stopPlayback();
    // 设置音量 (0-100)
    void setVolume(int volume);
    // 获取当前状态
    bool isPlaying() const;

    //  文件/URL 播放接口 (基于 QMediaPlayer)
    void playUrl(const QUrl& url);
    void playData(const QByteArray& data); // 播放完整的内存文件

    //  流式播放接口 (基于 QAudioSink + Worker Thread)
    /**
     * @brief 开启流式播放会话
     * @param sampleRate 采样率 (默认 32000)
     * @param channelCount 通道数 (默认 1)
     * @param bitDepth 位深 (默认 16)
     */
    void startStream(int sampleRate = 32000, int channelCount = 1, int bitDepth = 16);

    /**
     * @brief 写入流数据
     * @param chunk 音频数据块
     */
    void pushStreamData(const QByteArray& chunk);

    /**
     * @brief 结束流 (停止接收新数据，播放完当前缓冲后停止)
     */
    void stopStream();

signals:
    // 内部转发给 Worker 的信号
    void sigOperateStreamStart(int sampleRate, int channelCount, int bitDepth);
    void sigOperateStreamChunk(const QByteArray& chunk);
    void sigOperateStreamStop();

    // 对外通知信号
    void playbackFinished();
    void errorOccurred(const QString& error);

private:
    // 文件播放组件
    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;

    // 流式播放组件
    QThread* m_workerThread = nullptr;
    StreamAudioWorker* m_streamWorker = nullptr;

    // 状态管理
    bool m_isStreaming = false;
};