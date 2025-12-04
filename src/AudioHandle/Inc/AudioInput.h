//
// Created by Administrator on 2025/1/17.
//
#pragma once

#include <QAudioSource>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QTimer>
#include <QDir>
#include <vector>

/**
 * @brief  录音模块
 * @author Misaki
 * @date   2025/1/17(first) 2025/11/30(update)
 * 单例类
 * 使用 QAudioSource 获取原始 PCM 数据，实现 RMS 计算和 WAV 保存
 */
class AudioInput : public QObject
{
Q_OBJECT

private:
    /**
     * @brief  构造函数
     * @param parent
     */
    explicit AudioInput(QObject *parent = nullptr);
    static AudioInput* instance;

public:
    /**
     * @brief  获取实例
     * @return AudioInput*
     */
    static AudioInput* getInstance();
    /**
     * @brief  析构函数
     */
    ~AudioInput() override;

    /**
     * @brief 配置音频参数 (Qt6 中推荐使用 float 或 int16)
     */
    void setAudioSettings(int rate = 44100, int channels = 2);
    /**
     * @brief  设置录音文件输出路径与文件名
     * @param path       输出路径
     * @param fileName   文件名
     */
    void setAudioPath(const QString &path, const QString &fileName);

    /**
     * @brief  开始录音
     */
    void startAudio();

    /**
     * @brief  停止录音
     */
    void stopAudio();

    /**
     * @brief 设置录音时间并开始录音
     * @param duration 录音时长，单位为秒
     */
    void startAudioWithDuration(int duration);

    /**
     * @brief 开始自动录音，根据声音判断是否停止
     * @param silenceThreshold 静音阈值，低于该值则认为没有声音
     * @param silenceDuration 静音持续时间，单位为毫秒
     */
    void startAutoStopAudio(qreal silenceThreshold = 1200, int silenceDuration = 1500);

    /**
     * @brief 开始最佳阈值计算
     * @param Duration 持续时间，单位为毫秒
     */
    void startAutoThresholdClu(int Duration = 5000);

    /**
     * @brief 获取当前系统所有的音频输入设备
     * @return 音频输入设备名称列表
     */
    static QList<QString> getAvailableAudioInputDevices();
    /**
     * @brief 设置当前录音设备
     * @param deviceName 设备名称
     */
    void setAudioInputDevice(const QString &deviceName);

    /**
     * @brief 设置静音阈值
     * @param silenceThreshold 阈值
     */
    void setSilenceThreshold(qreal silenceThreshold);
    [[nodiscard]] qreal getSilenceThreshold() const;

private:
    // WAV头生成工具函数
    [[nodiscard]] QByteArray generateWavHeader(quint32 dataSize) const;
    // 计算RMS值工具函数
    static qreal calculateRMS(const QByteArray& buffer);

signals:
    // 录音完成信号
    void recordingFinished();
    void recordingFinished_Byte(const QByteArray &wavData);     // 携带音频数据
    // 实时RMS值信号
    void rmsRealValue(qreal value);
    // 阈值计算完成信号
    void thresholdCalculated(qreal bestThreshold);

private slots:
    void onTimeout();  // 定时器超时槽函数
    void thresholdTimeout();    // 阈值超时槽函数
    // void processBuffer(const QAudioBuffer& buffer); // 处理缓冲区数据
    void onReadyRead(); // 替代原先的 processBuffer，当有音频数据来时触发


private:
    QAudioSource *m_audioSource = nullptr;  /// Qt6 核心录音对象
    QIODevice *m_ioDevice = nullptr;        /// 用于读取数据的 IO 设备
    QAudioFormat m_format;                  /// 音频格式
    QAudioDevice m_currentDevice;           /// 当前选中的输入设备

    // 数据缓存
    QByteArray m_rawPCMData;                /// 存储原始PCM数据
    QString m_outputFilePath;

    // 逻辑控制变量
    bool isAutoRecording = false;           /// 是否自动录音状态
    bool isAutoThreshold = false;           /// 是否自动计算阈值
    qreal m_rmsValue = 0.0;                 /// 实时RMS值

    // 定时器
    QTimer *m_timer;                        /// 总时长定时器
    QTimer *m_silenceTimer;                 /// 静音检测定时器
    QTimer *m_thresholdTimer;               /// 阈值计算定时器

    // 阈值算法相关
    std::vector<qreal> m_rmsValues;         /// RMS值vector
    qreal m_silenceThreshold = 1200;        /// 静音阈值
    int m_silenceDuration = 1500;           /// 静音持续时间
};

