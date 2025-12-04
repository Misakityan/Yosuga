//
// Created by Administrator on 2025/1/17.
//
#pragma once

#include <QObject>
#include <QMediaPlayer>     // 音频播放模块
#include <QAudioOutput>     // QMediaPlayer 的音量控制组件
#include <QAudioSink>       // 音频输出组件， 用于原始数据播放
#include <QUrl>
#include <QBuffer>

/**
 * @brief 音频播放模块
 * @author Misaki
 * 单例类
 * 本模块重新基于Qt6重构
 * 实现的功能
 * 1. 设定传入的音频文件路径
 * 2. 根据音频文件路径播放音频
 * 将上面的两个函数封装成一个槽函数，以及设定一个对应的信号
 */
class AudioOutput : public QObject
{
Q_OBJECT
private:
    /**
     * 构造函数私有化
     * @param parent
     */
    explicit AudioOutput(QObject *parent = nullptr);        // 并不将本模块挂在对象树当中，因为本模块为单例类，内存自行管理

    static AudioOutput *instance;       // 单例类
public:
    static AudioOutput *getInstance();

    /**
     * 析构函数
     */
    ~AudioOutput() override;

    /**
     * 播放来自文件的音频
     * @param url
     */
    void playAudio(const QUrl& url) const;

    /**
     * 播放内存中的WAV字节流数据
     * @param wavData 完整的WAV格式字节流
     */
    void playFromByteArray(const QByteArray &wavData);

    /**
     * 获取WAV音频格式
     * @param wavData
     * @return
     */
    QAudioFormat getWavFormat(const QByteArray &wavData) const;

    /**
     * 暂停播放音频
     */
    void pauseAudio() const;

    /**
     * 停止播放音频
     */
    void stopAudio() const;

    /**
     * 设置播放速度
     * @param speed
     */
    void setPlaySpeed(double speed) const;

    /**
     * 获取播放速度
     * @return double
     */
    [[nodiscard]] double getPlaySpeed() const;

    /**
     * 设置播放音量
     * @param volume
     */
    void setPlayVolume(int volume) const;

    /**
     * 获取播放音量
     * @return int
     */
    [[nodiscard]] int getPlayVolume() const;

    /**
     * 获取当前播放位置（毫秒）
     * @return qint64
     */
    [[nodiscard]] qint64 getPlayPosition() const;

    /**
     * 获取媒体总时长（毫秒）
     * @return qint64
     */
    [[nodiscard]] qint64 getMediaDuration() const;

    /**
     * 根据当前播放位置，返回播放进度百分比
     * @return double
     */
    [[nodiscard]] double getPlayProgress() const;

    void setAudioFormat(int sampleRate, int channels, QAudioFormat::SampleFormat sampleType);

    void setAudioFormat(const QAudioFormat &format_);

    [[nodiscard]] QAudioFormat getAudioFormat() const;

    /**
     * 获取播放状态
     * QMediaPlayer::StoppedState、QMediaPlayer::PlayingState、QMediaPlayer::PausedState
     * @return QMediaPlayer::State
     */
    [[nodiscard]] QMediaPlayer::MediaStatus getState() const;

    /**
     * 获取错误类型
     *  NoError,
        ResourceError,
        FormatError,
        NetworkError,
        AccessDeniedError
     * @return QMediaPlayer::Error
     */
    [[nodiscard]] QMediaPlayer::Error getError() const;

    /**
     * 获取错误描述
     * @return QString
     */
    [[nodiscard]] QString getErrorString() const;

signals:
    void playbackFinished(); // 播放完成信号

private:
    QMediaPlayer *mediaPlayer;  /// <!音频播放核心组件
    QAudioOutput *audioOutput;  /// <!音量和设备控制
    QAudioSink *audioSink;      /// <!字节流播放组件
    QAudioFormat format;        /// <!字节流播放时所用音频格式
    QBuffer *audioBuffer;       /// <!存储内存数据
};
