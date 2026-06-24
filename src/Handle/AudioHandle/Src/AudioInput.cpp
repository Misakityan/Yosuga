//
// Created by Administrator on 2025/1/17.
//

#include "AudioInput.h"
#include <QDebug>
#include <QtMath>
#include <QtEndian> // 用于处理字节序

QScopedPointer<AudioInput> AudioInput::instance;
QMutex AudioInput::mutex;
AudioInput *AudioInput::getInstance()
{
    // 懒汉式 依旧单线程无需加锁
    if (instance.isNull()) {
        QMutexLocker locker(&mutex);
        if (instance.isNull()) {
            instance.reset(new AudioInput);
        }
    }
    return instance.data();
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
#ifdef EMBEDDED_LINUX
    m_rawPCMData.clear();
    if (!m_arecordProcess) {
        m_arecordProcess = new QProcess(this);
        connect(m_arecordProcess, &QProcess::readyReadStandardOutput, this, [this]() {
            m_rawPCMData.append(m_arecordProcess->readAllStandardOutput());
        });
    } else if (m_arecordProcess->state() != QProcess::NotRunning) {
        m_arecordProcess->terminate();
        m_arecordProcess->waitForFinished(1000);
    }
    m_arecordProcess->start("arecord", {
        "-D", "hw:0,0", "-f", "S16_LE",
        "-r", QString::number(m_format.sampleRate()),
        "-c", QString::number(m_format.channelCount()),
        "-t", "raw", "-q", "-"
    });
    if (m_arecordProcess->waitForStarted(1000)) {
        qDebug() << "Started recording via arecord: hw:0,0"
                 << m_format.sampleRate() << "Hz" << m_format.channelCount() << "ch S16_LE";
    } else {
        qCritical() << "Failed to start arecord";
    }
#else
    // 每次开始前重新创建 QAudioSource，确保状态重置
    if (m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }

    m_audioSource = new QAudioSource(m_currentDevice, m_format, this);
    m_audioSource->setBufferSize(128000);

    // start() 返回一个 QIODevice，可以从中读取数据
    m_ioDevice = m_audioSource->start();

    if (m_ioDevice) {
        connect(m_ioDevice, &QIODevice::readyRead, this, &AudioInput::onReadyRead);
        qDebug() << "Started recording with device:" << m_currentDevice.description();
    } else {
        qCritical() << "Failed to start audio recording.";
    }
#endif
}

void AudioInput::stopAudio()
{
#ifdef EMBEDDED_LINUX
    if (m_arecordProcess && m_arecordProcess->state() != QProcess::NotRunning) {
        m_arecordProcess->terminate();
        m_arecordProcess->waitForFinished(2000);
    }
#else
    if (m_audioSource) {
        m_audioSource->stop();
    }
#endif

    // 停止所有定时器
    m_timer->stop();
    m_silenceTimer->stop();
    m_thresholdTimer->stop();

    // 生成 WAV 数据
    QByteArray wavData;
    if (!m_rawPCMData.isEmpty()) {
        wavData = generateWavHeader(m_rawPCMData.size());
        wavData.append(m_rawPCMData);
#ifdef QT_DEBUG
        // 如果需要保存文件(Debug下启用)
        if (!m_outputFilePath.isEmpty()) {
            QFile file(m_outputFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(wavData);
                file.close();
                qDebug() << "Saved WAV to:" << m_outputFilePath;
            }
        }
#endif
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

    // 保存原始 PCM 数据
    m_rawPCMData.append(data);

    // 计算 RMS (仅用于分析，计算当前块的RMS)
    const qreal currentRms = calculateRMS(data);
    m_rmsValue = currentRms;
    // 计算平滑RMS (用于防止低频杂波突然打断静音检测)
    constexpr qreal alpha = 0.15;    // 85% 历史权重, 15% 当前权重
    if (qFuzzyIsNull(m_smoothRms)) {
        // 如果是第一帧数据，直接赋值，避免从0开始慢慢爬升
        m_smoothRms = currentRms;
    } else {
        // 新值 = (旧值 * (1 - alpha)) + (当前值 * alpha)
        m_smoothRms = (m_smoothRms * (1.0 - alpha)) + (currentRms * alpha);
    }

    // 自动停止逻辑 (VAD)
    if (isAutoRecording) {
        // 输出 RMS 用于调试
        qDebug() << "Raw:" << currentRms << " Smooth:" << m_smoothRms;

        if (m_smoothRms < m_silenceThreshold) {
            // [当前是静音]

            // 如果之前已经检测到过人声（说明是话说完了，或者是句间停顿）
            if (m_hasVoiceDetected) {
                // 启动/保持“短时”静音检测 (由 AppCore 传入，例如 500ms 或 1500ms)
                if (!m_silenceTimer->isActive()) {
                    m_silenceTimer->start(m_silenceDuration);
                }
                // 如果 Timer 正在运行，就让它继续倒计时，超时会自动触发 stopAudio
            }
            else {
                // [还没有检测到过人声] (起始静音)
                // 这里不需要做额外操作，startAutoStopAudio 里设置的 5000ms 长定时器在跑
                // 允许用户深呼吸或准备
            }
        } else {
            // [当前有声音]
            m_hasVoiceDetected = true; // 标记：已经有人说话了

            // 重置静音定时器
            // 只要有人说话，就不断重置定时器，防止断录
            m_silenceTimer->stop();
            // 这里可以预设启动，也可以不启动，只要有声音就会一直 stop
            // 为了安全，设为 silenceDuration
            m_silenceTimer->start(m_silenceDuration);
        }
    }

    // 自动阈值计算逻辑
    if (isAutoThreshold) {
        m_rmsValues.push_back(m_smoothRms);
        emit rmsRealValue(m_smoothRms);
    }
}

qreal AudioInput::calculateRMS(const QByteArray& buffer)
{
    if (buffer.isEmpty()) return 0;

    // 设定为 Int16 格式 (16位深)
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

    // 重置状态
    m_hasVoiceDetected = false;
    m_smoothRms = 0.0;
    startAudio();

    // 延迟200ms是为了避开硬件启动时的爆音，但不需要立即启动短时倒计时
    // 延迟启动静音检测，给一点缓冲时间
    QTimer::singleShot(200, this, [this](){
        if(isAutoRecording) { // 确保还在录音状态
             // 如果还没检测到声音，给5秒的等待时间；如果检测到了，逻辑由onReadyRead接管
             if(!m_hasVoiceDetected) {
                 m_silenceTimer->start(5000); // 5秒没声音就停止
             }
        }
    });
}

// 启动阈值计算
void AudioInput::startAutoThresholdClu(const int Duration)
{
    isAutoThreshold = true;
    m_rmsValues.clear();
    startAudio();
    m_thresholdTimer->start(Duration);
}

/**
 * 2025.12.30重构 Misaki
 * 从均值阈值计算的基础上增加了N倍标准差
 * 即阈值 = 均值 + N * 标准差(N取3)
 */
void AudioInput::thresholdTimeout()
{
    isAutoThreshold = false;
    stopAudio(); // 内部会处理 stop

    if (m_rmsValues.empty()) {
        emit thresholdCalculated(0);
        return;
    }
    // 计算均值
    const double mean = std::accumulate(m_rmsValues.begin(), m_rmsValues.end(), 0.0) / m_rmsValues.size();
    // 计算标准差
    const double sq_sum = std::inner_product(m_rmsValues.begin(), m_rmsValues.end(), m_rmsValues.begin(), 0.0);
    double variance = (sq_sum / m_rmsValues.size()) - (mean * mean);
    // 防止浮点误差导致负数
    if (variance < 0) variance = 0;
    const double stdDev = std::sqrt(variance);

    // 增加一个固定的偏移量 offset
    // 确保即使环境有轻微波动，也不会触发录音
    constexpr double offset = 80.0;
    // 阈值 = 均值 + 2 * 标准差
    const double bestThreshold = mean + (3 * stdDev) + offset;
    m_silenceThreshold = std::max(bestThreshold, 150.0);
    m_silenceThreshold = std::min(m_silenceThreshold, 30000.0);
    qDebug() << "Auto Threshold Calc -> Mean:" << mean
             << " StdDev:" << stdDev
             << " Result:" << m_silenceThreshold;
    emit thresholdCalculated(m_silenceThreshold);
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
    header.bitsPerSample = 16; // 强制使用了 Int16

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