//
// Created by Administrator on 2025/1/17.
//

#include "AudioOutput.h"
#include <QMediaDevices>
#include <QDataStream>

AudioOutput *AudioOutput::instance = nullptr;

AudioOutput *AudioOutput::getInstance()
{
    // 懒汉式(单线程播放，无需考虑加锁)
    if (instance == nullptr) {
        instance = new AudioOutput();
    }
    return instance;
}

AudioOutput::AudioOutput(QObject *parent) : QObject(parent), mediaPlayer(nullptr), audioOutput(nullptr), audioSink(nullptr), audioBuffer(nullptr)
{
    audioBuffer = new QBuffer(this); // 初始化缓冲区
    // 初始化 QMediaPlayer 用于文件播放
    mediaPlayer = new QMediaPlayer(this);
    // 初始化QAudioOutput 用于控制 QMediaPlayer 的音量与设备
    audioOutput = new QAudioOutput(QMediaDevices::defaultAudioOutput(), this);
    mediaPlayer->setAudioOutput(audioOutput);   // 将 QMediaPlayer 与 QAudioOutput 关联起来

    // 监听 QMediaPlayer 状态变化
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus state) {
        if (state == QMediaPlayer::EndOfMedia) {
            emit playbackFinished();        // 触发播放完成信号
        }
    });

    // 初始化 默认的QAudioSink 和 QBuffer 用于字节流播放
    format.setSampleRate(44100);    // 采样率
    format.setChannelCount(2);      // 播放通道数
    format.setSampleFormat(QAudioFormat::Int16);    // 采样格式
    audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this);
    audioBuffer = new QBuffer(this);

}


AudioOutput::~AudioOutput()
{
    if (mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        mediaPlayer->stop();
    }
    if (audioSink->state() != QAudio::StoppedState) {
        audioSink->stop();
    }
}

void AudioOutput::playAudio(const QUrl& url) const {
    mediaPlayer->setSource(url);
    mediaPlayer->play();
}

/**
 * 从字节数组中播放音频
 * @param wavData
 */
void AudioOutput::playFromByteArray(const QByteArray &wavData) {
    // 确保数据有效
    if (wavData.isEmpty()) {
        qWarning() << "尝试播放空音频数据";
        return;
    }
    // 停止 QMediaPlayer 播放，防止冲突
    if (mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        mediaPlayer->stop();
    }
    // 停止并释放旧的 QAudioSink   这里因为QAudioSink 不支持运行时修改播放音频格式，因此只能这么做
    if (audioSink) {
        audioSink->stop();
        delete audioSink;
        audioSink = nullptr;
    }
    // 解析新的音频格式
    const QAudioFormat newFormat = getWavFormat(wavData);
    if (!newFormat.isValid()) {
        qWarning() << "音频格式解析失败，停止播放。";
        return;
    }
    // 创建一个新的 QAudioSink 实例
    audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), newFormat, this);
    // 准备 QBuffer：只包含音频原始数据 (跳过 44 字节的 WAV 头部)
    const QByteArray rawAudioData = wavData.mid(44);

    // 重置缓冲区
    audioBuffer->close();
    audioBuffer->setData(rawAudioData);
    audioBuffer->open(QIODevice::ReadOnly);

    // 监听 QAudioSink 状态变化以触发 playbackFinished 信号
    connect(audioSink, &QAudioSink::stateChanged, this, [this](const QAudio::State state) {
        if (state == QAudio::StoppedState) {
            if (audioSink->error() == QAudio::NoError) {
                emit playbackFinished();
            } else {
                qWarning() << "QAudioSink 播放错误:" << audioSink->error();
            }
        }
    });

    // 将 QBuffer (QIODevice) 传递给 QAudioSink，并开始播放
    audioSink->start(audioBuffer);
}

/**
 * @brief 从完整的 WAV 字节流中解析 QAudioFormat
 * @param wavData 完整的 WAV 文件字节流
 * @return QAudioFormat 如果解析成功，返回正确的格式；否则返回默认格式
 */
QAudioFormat AudioOutput::getWavFormat(const QByteArray &wavData) const {
    QAudioFormat format_;

    // 完整的 WAV 文件至少有 44 字节的头部
    if (wavData.size() < 44) {
        qWarning() << "WAV 数据太短，无法解析头部";
        return this->format; // 返回一个默认格式
    }

    // 使用 QDataStream 以小端模式读取二进制数据
    QDataStream stream(wavData);
    stream.setByteOrder(QDataStream::LittleEndian);

    // 跳过 RIFF 和 FORMAT 块 (共 20 字节)
    stream.skipRawData(20);

    // 读取声道数 (2 bytes)
    quint16 channels = 0;
    stream >> channels;
    format_.setChannelCount(channels);

    // 读取采样率 (4 bytes)
    quint32 sampleRate = 0;
    stream >> sampleRate;
    format_.setSampleRate(sampleRate);

    // 跳过 ByteRate 和 BlockAlign (共 6 字节)
    stream.skipRawData(6);

    // 读取位深 (2 bytes)
    quint16 bitsPerSample = 0;
    stream >> bitsPerSample;

    // 根据位深设置 QAudioFormat 的 SampleFormat
    if (bitsPerSample == 8) {
        format_.setSampleFormat(QAudioFormat::UInt8);
    } else if (bitsPerSample == 16) {
        format_.setSampleFormat(QAudioFormat::Int16);
    } else if (bitsPerSample == 32) {
        format_.setSampleFormat(QAudioFormat::Float); // 32位通常是浮点数
    } else {
        qWarning() << "不支持的位深:" << bitsPerSample;
        return this->format;        // 返回一个默认格式
    }

    qDebug() << "WAV Format - Rate:" << sampleRate << ", Channels:" << channels << ", Bits:" << bitsPerSample;
    return format_;
}

/**
 * 暂停播放
 */
void AudioOutput::pauseAudio() const {
    mediaPlayer->pause();
}

/**
 * 停止播放
 */
void AudioOutput::stopAudio() const {
    mediaPlayer->stop();
}

/**
 * 设置播放速度
 * @param speed
 */
void AudioOutput::setPlaySpeed(const double speed) const {
    mediaPlayer->setPlaybackRate(speed);
}


/**
 * 获取播放速度
 * @return double
 */
double AudioOutput::getPlaySpeed() const {
    return mediaPlayer->playbackRate();
}

/**
 * 设置播放音量
 * @param volume
 */
void AudioOutput::setPlayVolume(const int volume) const {
    // 将 0-100 转换为 0.0 - 1.0
    const qreal newVolume = static_cast<qreal>(volume) / 100.0;
    audioOutput->setVolume(static_cast<float>(newVolume));
}

/**
 * 获取播放音量
 * @return int
 */
int AudioOutput::getPlayVolume() const {
    // 将 0.0 - 1.0 转换为 0-100
    return qRound(audioOutput->volume() * 100);
}

/**
 * 获取当前播放位置（毫秒）
 * @return qint64
 */
qint64 AudioOutput::getPlayPosition() const {
    // 检查 QMediaPlayer 是否处于播放状态，且当前是否有源在播放
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        return mediaPlayer->position();
    }
    // 否则返回 0
    return 0;
}

/**
 * 获取音频时长（毫秒）
 * @return qint64
 */
qint64 AudioOutput::getMediaDuration() const {
    if(mediaPlayer->playbackState() == QMediaPlayer::PlayingState){
        return mediaPlayer->duration();
    }
    return 0;
}

/**
 * 根据当前播放位置，返回播放进度百分比
 * @return double
 */
double AudioOutput::getPlayProgress() const {
    if(mediaPlayer->playbackState() != QMediaPlayer::PlayingState){
        return 0;
    }
    return static_cast<double>(this->getPlayPosition()) / static_cast<double>(this->getMediaDuration());
}

void AudioOutput::setAudioFormat(const int sampleRate, const int channels, const QAudioFormat::SampleFormat sampleType) {
    this->format.setSampleRate(sampleRate);
    this->format.setChannelCount(channels);
    this->format.setSampleFormat(sampleType);
}

void AudioOutput::setAudioFormat(const QAudioFormat &format_) {
    this->format = format_;
}

QAudioFormat AudioOutput::getAudioFormat() const {
    return this->format;
}


/**
 * 获取播放状态
 * QMediaPlayer::StoppedState、QMediaPlayer::PlayingState、QMediaPlayer::PausedState
 * @return QMediaPlayer::State
 */
QMediaPlayer::MediaStatus AudioOutput::getState() const {
    return mediaPlayer->mediaStatus();
}

/**
 * 获取错误类型
 *  NoError,
    ResourceError,
    FormatError,
    NetworkError,
    AccessDeniedError,
    ServiceMissingError,
    MediaIsPlaylist
 * @return QMediaPlayer::Error
 */
QMediaPlayer::Error AudioOutput::getError() const {
    return mediaPlayer->error();
}

/**
 * 获取错误描述
 * @return QString
 */
QString AudioOutput::getErrorString() const {
    return mediaPlayer->errorString();
}