//
// Created by Administrator on 2025/1/17.
//

#include "AudioOutput.h"
#include <QMediaDevices>
#include <QDebug>
#include <QCoreApplication>
#include <QDataStream>

//  Static Helpers (WAV Header Parser)
static bool hasWavHeader(const QByteArray& data) {
    if (data.size() < 44) return false;
    return data.startsWith("RIFF") && data.mid(8, 4) == "WAVE";
}

//  StreamAudioWorker 实现

StreamAudioWorker::~StreamAudioWorker() {
    // 确保析构时资源释放
    stop();
}

void StreamAudioWorker::start(int sampleRate, int channelCount, int bitDepth) {
    if (m_sink) {
        m_sink->stop();
        m_sink.reset();
    }

    m_firstChunk = true;

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);

    if (bitDepth == 8) format.setSampleFormat(QAudioFormat::UInt8);
    else if (bitDepth == 16) format.setSampleFormat(QAudioFormat::Int16);
    else if (bitDepth == 32) format.setSampleFormat(QAudioFormat::Float);
    else format.setSampleFormat(QAudioFormat::Int16);

    auto device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        qWarning() << "[Worker] Device does not support format, using preferred format.";
        format = device.preferredFormat();
    }

    m_sink.reset(new QAudioSink(device, format));
    m_sink->setBufferSize(sampleRate * channelCount * (bitDepth / 8));

    connect(m_sink.data(), &QAudioSink::stateChanged, this, [this](QAudio::State state){
        if (state == QAudio::IdleState) {
            emit playbackFinished();
        }
        else if (state == QAudio::StoppedState) {
            if (m_sink->error() != QAudio::NoError) {
                emit errorOccurred("Audio Sink Error: " + QString::number(m_sink->error()));
            }
        }
    });

    m_ioDevice = m_sink->start();
    if (!m_ioDevice) {
        emit errorOccurred("Failed to start audio device");
    } else {
        qDebug() << "[Worker] Stream started:" << sampleRate << "Hz, buffer:" << m_sink->bufferSize();
    }
}

void StreamAudioWorker::processChunk(const QByteArray& chunk) {
    if (chunk.isEmpty() || !m_ioDevice || !m_sink) return;
    QByteArray dataToWrite = chunk;
    if (m_firstChunk) {
        if (hasWavHeader(chunk)) {
            dataToWrite = chunk.mid(44);
        }
        m_firstChunk = false;
    }

    const char *ptr = dataToWrite.constData();
    qint64 remaining = dataToWrite.size();
    while (remaining > 0) {
        qint64 written = m_ioDevice->write(ptr, remaining);
        if (written > 0) {
            ptr += written;
            remaining -= written;
        } else {
            QThread::msleep(5);
        }
    }
}

void StreamAudioWorker::stop() {
    if (m_sink) {
        m_sink->stop();
        m_sink.reset(); // 删除对象
    }
    m_ioDevice = nullptr;
    qDebug() << "[Worker] Stream stopped";
}

//  AudioOutput 主类实现

QScopedPointer<AudioOutput> AudioOutput::m_instance;
QMutex AudioOutput::m_mutex;

AudioOutput* AudioOutput::getInstance() {
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            m_instance.reset(new AudioOutput());
        }
    }
    return m_instance.data();
}

void AudioOutput::destroy() {
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset();
    }
}

AudioOutput::AudioOutput(QObject *parent) : QObject(parent) {
    // 初始化文件播放器
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia) emit playbackFinished();
    });

    // 预先初始化流式 Worker 线程
    // 保持一个常驻线程Worker，通过信号控制
    m_streamWorker = new StreamAudioWorker(); // 不能指定 parent，因为要 moveToThread
    m_workerThread = new QThread(this);

    m_streamWorker->moveToThread(m_workerThread);

    // 连接信号槽
    // 主线程 -> Worker
    connect(this, &AudioOutput::sigOperateStreamStart, m_streamWorker, &StreamAudioWorker::start);
    connect(this, &AudioOutput::sigOperateStreamChunk, m_streamWorker, &StreamAudioWorker::processChunk);
    connect(this, &AudioOutput::sigOperateStreamStop, m_streamWorker, &StreamAudioWorker::stop);

    // Worker -> 主线程
    connect(m_streamWorker, &StreamAudioWorker::errorOccurred, this, &AudioOutput::errorOccurred);

    // 线程启动
    m_workerThread->start();
}

AudioOutput::~AudioOutput() {
    stopPlayback();

    // 清理线程
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(3000); // 等待退出
        delete m_streamWorker;
    }
}

//  对外接口

void AudioOutput::stopPlayback() {
    // 停止文件播放
    if (m_player->playbackState() != QMediaPlayer::StoppedState) {
        m_player->stop();
    }

    // 停止流播放
    if (m_isStreaming) {
        stopStream();
    }
}

void AudioOutput::setVolume(int volume) {
    if (m_audioOutput) m_audioOutput->setVolume(volume / 100.0);
    // 注意：流式播放的音量控制需要在 Worker 内单独实现
}

bool AudioOutput::isPlaying() const {
    return (m_player->playbackState() == QMediaPlayer::PlayingState) || m_isStreaming;
}

// 文件播放
void AudioOutput::playUrl(const QUrl& url) {
    stopPlayback(); // 互斥，播放新文件前停止旧的
    m_player->setSource(url);
    m_player->play();
}

void AudioOutput::playData(const QByteArray& data) {
    // 这个方法对于 QMediaPlayer 比较麻烦，需要自定义 QIODevice
    // 建议直接走 stream 接口，或者使用 QBuffer + StreamWorker 的一次性模式
    // 为了简单，这里将 buffer 视为 stream 播放
    stopPlayback();
    startStream(44100, 2, 16); // 假设默认 wav 格式，Worker 会自动解析头
    pushStreamData(data);
    // 不需要显式 stopStream，让它播完
}

// 流式播放

void AudioOutput::startStream(int sampleRate, int channelCount, int bitDepth) {
    stopPlayback(); // 确保干净的状态
    m_isStreaming = true;

    // 通过信号跨线程调用 Worker 的 start
    emit sigOperateStreamStart(sampleRate, channelCount, bitDepth);
}

void AudioOutput::pushStreamData(const QByteArray& chunk) {
    if (!m_isStreaming) return;

    // 直接发射信号，Qt 会把 chunk copy 到子线程事件队列
    emit sigOperateStreamChunk(chunk);
}

void AudioOutput::stopStream() {
    if (!m_isStreaming) return;

    m_isStreaming = false;
    emit sigOperateStreamStop();
}