//
// Created by Administrator on 2025/1/17.
//

#include "AudioInput.h"
#include <QDebug>
#include <QtMath>
#include <QtEndian> // 用于处理字节序

AudioInput *AudioInput::instance = nullptr;
AudioInput *AudioInput::getInstance()
{
    // 懒汉式 依旧单线程无需加锁
    if (instance == nullptr) {
        instance = new AudioInput();
    }
    return instance;
}

AudioInput::AudioInput(QObject *parent) : QObject(parent)
{
    // new一些必要的对象
    // 初始化定时器
    m_timer = new QTimer(this);
    m_silenceTimer = new QTimer(this);
    m_thresholdTimer = new QTimer(this);
    m_thresholdTimer->setSingleShot(true);

    // 连接定时器信号
    connect(m_timer, &QTimer::timeout, this, &AudioInput::onTimeout);                   // 录音超时槽函数
    connect(m_silenceTimer, &QTimer::timeout, this, &AudioInput::stopAudio);            // 录音超时槽函数
    connect(m_thresholdTimer, &QTimer::timeout, this, &AudioInput::thresholdTimeout);   // 阈值检测超时槽函数

    // 初始化默认设备和格式
    m_currentDevice = QMediaDevices::defaultAudioInput();
    setAudioSettings(); // 使用默认参数
}

AudioInput::~AudioInput()
{
    stopAudio();    // 停止录音
    if (m_audioSource) {
        delete m_audioSource;
    }
}


void AudioInput::setAudioSettings(const int rate, const int channels)
{
    m_format.setSampleRate(rate);
    m_format.setChannelCount(channels);
    // 重要：Qt6 默认可能是 float，为了生成标准 WAV 且方便计算 RMS，强制设为 Int16
    m_format.setSampleFormat(QAudioFormat::Int16);

    // 检查设备是否支持该格式，不支持则使用最接近的
    if (!m_currentDevice.isFormatSupported(m_format)) {
        qWarning() << "Requested format not supported, using preferred format.";
        m_format = m_currentDevice.preferredFormat();
    }
}


void AudioInput::setAudioPath(const QString &path, const QString &fileName)
{
    this->m_outputFilePath = path + fileName;
}


void AudioInput::startAudio()
{
    // 每次开始前重新创建 QAudioSource，确保状态重置
    if (m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }

    m_audioSource = new QAudioSource(m_currentDevice, m_format, this);

    // 调大缓冲区以避免溢出
    m_audioSource->setBufferSize(128000);

    // start() 返回一个 QIODevice，我们可以从中读取数据
    m_ioDevice = m_audioSource->start();

    if (m_ioDevice) {
        connect(m_ioDevice, &QIODevice::readyRead, this, &AudioInput::onReadyRead);
        qDebug() << "Started recording with device:" << m_currentDevice.description();
    } else {
        qCritical() << "Failed to start audio recording.";
    }
}

void AudioInput::stopAudio()
{
    if (m_audioSource) {
        m_audioSource->stop();
        // 注意：不要立即 delete m_audioSource，某些情况下可能导致 crash，停止即可
    }

    // 停止所有定时器
    m_timer->stop();
    m_silenceTimer->stop();
    m_thresholdTimer->stop();

    // 生成 WAV 数据
    QByteArray wavData;
    if (!m_rawPCMData.isEmpty()) {
        wavData = generateWavHeader(m_rawPCMData.size());
        wavData.append(m_rawPCMData);

        // 如果需要保存文件
        if (!m_outputFilePath.isEmpty()) {
            QFile file(m_outputFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(wavData);
                file.close();
                qDebug() << "Saved WAV to:" << m_outputFilePath;
            }
        }

        m_rawPCMData.clear();
    }

    isAutoRecording = false;
    isAutoThreshold = false;

    emit recordingFinished();
    emit recordingFinished_Byte(wavData);
    qDebug() << "Recording stopped.";
}

// 阈值检测超时槽函数
void AudioInput::onReadyRead()
{
    if (!m_ioDevice) return;

    // 读取当前所有可用的音频数据
    QByteArray data = m_ioDevice->readAll();
    if (data.isEmpty()) return;

    // 1. 保存原始 PCM 数据
    m_rawPCMData.append(data);

    // 2. 计算 RMS (仅用于分析，取最后一小段或者整体计算，这里计算当前块的RMS)
    m_rmsValue = calculateRMS(data);

    // 3. 自动停止逻辑 (VAD)
    if (isAutoRecording) {
        // 输出 RMS 用于调试
        // qDebug() << "RMS:" << m_rmsValue;

        if (m_rmsValue < m_silenceThreshold) {
            // 静音状态
            if (!m_silenceTimer->isActive()) {
                m_silenceTimer->start(m_silenceDuration);
            }
        } else {
            // 有声音，重置定时器
            m_silenceTimer->stop();
            m_silenceTimer->start(m_silenceDuration);
        }
    }

    // 4. 自动阈值计算逻辑
    if (isAutoThreshold) {
        m_rmsValues.push_back(m_rmsValue);
        emit rmsRealValue(m_rmsValue);
    }
}

qreal AudioInput::calculateRMS(const QByteArray& buffer)
{
    if (buffer.isEmpty()) return 0;

    // 假设是 Int16 格式 (16位深)
    // 如果是 Stereo，数据排列是 L R L R...
    // 简单的 RMS 计算可以将所有通道数据视为一个长序列

    const qint16 *data = reinterpret_cast<const qint16*>(buffer.constData());
    const int sampleCount = buffer.size() / sizeof(qint16); // 样本数量

    if (sampleCount == 0) return 0;

    qreal sumSquared = 0;
    for (int i = 0; i < sampleCount; ++i) {
        const qreal sample = static_cast<qreal>(data[i]);
        sumSquared += sample * sample;
    }

    return qSqrt(sumSquared / sampleCount);
}

// 启动带时长的录音
void AudioInput::startAudioWithDuration(int duration)
{
    startAudio();
    m_timer->start(duration * 1000);
}

void AudioInput::onTimeout()
{
    stopAudio();
    qDebug() << "Recording stopped by duration timeout.";
}

// 获取所有音频输入设备
QList<QString> AudioInput::getAvailableAudioInputDevices()
{
    QList<QString> list;
    const auto devices = QMediaDevices::audioInputs();
    for (const auto &device : devices) {
        list.append(device.description());
    }
    return list;
}

// 设置当前录音设备
void AudioInput::setAudioInputDevice(const QString &deviceName)
{
    const auto devices = QMediaDevices::audioInputs();
    for (const auto &device : devices) {
        if (device.description() == deviceName) {
            m_currentDevice = device;
            break;
        }
    }
}

// 启动自动录音 (VAD)
void AudioInput::startAutoStopAudio(const qreal silenceThreshold, const int silenceDuration)
{
    isAutoRecording = true;
    m_silenceThreshold = silenceThreshold;
    m_silenceDuration = silenceDuration;

    startAudio();

    // 延迟启动静音检测，给一点缓冲时间
    QTimer::singleShot(200, this, [this](){
        m_silenceTimer->start(m_silenceDuration);
    });
}

// 启动阈值计算
void AudioInput::startAutoThresholdClu(int Duration)
{
    isAutoThreshold = true;
    m_rmsValues.clear();
    startAudio();
    m_thresholdTimer->start(Duration);
}

void AudioInput::thresholdTimeout()
{
    isAutoThreshold = false;
    stopAudio(); // 内部会处理 stop

    if (!m_rmsValues.empty()) {
        const double sum = std::accumulate(m_rmsValues.begin(), m_rmsValues.end(), 0.0);
        const double avg = sum / m_rmsValues.size();
        m_silenceThreshold = avg + 500.0; // 这里的 500 是经验值，可以根据需要调整
        emit thresholdCalculated(m_silenceThreshold);
    } else {
        emit thresholdCalculated(0);
    }
}

QByteArray AudioInput::generateWavHeader(const quint32 dataSize) const {
    // WAV头结构定义
    struct WavHeader {
        char     riff[4] = {'R','I','F','F'};
        quint32  chunkSize;
        char     wave[4] = {'W','A','V','E'};
        char     fmt[4] = {'f','m','t',' '};
        quint32  fmtSize = 16;
        quint16  audioFormat = 1; // PCM
        quint16  numChannels;
        quint32  sampleRate;
        quint32  byteRate;
        quint16  blockAlign;
        quint16  bitsPerSample;
        char     data[4] = {'d','a','t','a'};
        quint32  dataSize;
    } header;

    header.numChannels = static_cast<quint16>(m_format.channelCount());
    header.sampleRate = static_cast<quint32>(m_format.sampleRate());
    header.bitsPerSample = 16; // 我们强制使用了 Int16

    header.byteRate = header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
    header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
    header.dataSize = dataSize;
    header.chunkSize = 36 + dataSize;

    return QByteArray(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
}

void AudioInput::setSilenceThreshold(const qreal silenceThreshold)
{
    this->m_silenceThreshold = silenceThreshold;
}

qreal AudioInput::getSilenceThreshold() const
{
    return this->m_silenceThreshold;
}