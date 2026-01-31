//
// Created by misaki on 2026/1/13.
//

/**
 * 数据传输对象 (DTO) 定义
 * AudioDataTransferObject
 * 与Yosuga_server对等
 */

#pragma once
#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include "DataTransferObjectBase.h"
// 前向声明，减少依赖
class QJsonObject;
class AudioDataTransferObject final : public DataTransferObjectBase{
public:
    // 构造函数（带默认值）
    explicit AudioDataTransferObject(QString  owner = "client",
                                   bool isStream = false,
                                   bool isStart = false,
                                   bool isEnd = false,
                                   int sequence = 0,
                                   QByteArray  data = {},
                                   int sampleRate = 16000,
                                   int channelCount = 1,
                                   int bitDepth = 16,
                                   double duration = 0.0,
                                   QString  text = "");
    // 静态工厂方法
    static AudioDataTransferObject fromJson(const QJsonObject& json);

    [[nodiscard]] QString type() const override { return "audio_data"; }

    // 序列化
    [[nodiscard]] QJsonObject toJson() const override;    // 通过多态即可统一调用方式

    // 链式调用设置
    AudioDataTransferObject& setData(const QString& key, const QJsonValue& value) override;

    [[nodiscard]] QString owner() const { return m_owner; }
    [[nodiscard]] bool isStream() const { return m_isStream; }
    [[nodiscard]] bool isStart() const { return m_isStart; }
    [[nodiscard]] bool isEnd() const { return m_isEnd; }
    [[nodiscard]] int sequence() const { return m_sequence; }
    [[nodiscard]] QByteArray audioData() const { return m_data; }
    [[nodiscard]] int sampleRate() const { return m_sampleRate; }
    [[nodiscard]] int channelCount() const { return m_channelCount; }
    [[nodiscard]] int bitDepth() const { return m_bitDepth; }
    [[nodiscard]] double duration() const { return m_duration; }
    [[nodiscard]] QString text() const { return m_text; }

private:
    QString m_owner;            /// 音频数据的拥有者(server or client)
    bool m_isStream;            /// 音频数据是否为流式数据
    bool m_isStart;             /// 音频数据是否开始(流式时有效)
    bool m_isEnd;               /// 音频数据是否结束(流式时有效)
    int m_sequence;             /// 音频数据块序列号(流式时有效)
    QByteArray m_data;          /// 音频数据，流式时为分块数据，base64编码
    int m_sampleRate;           /// 音频采样率
    int m_channelCount;         /// 音频通道数
    int m_bitDepth;             /// 音频采样位数
    double m_duration;          /// 音频时长
    QString m_text;             /// 音频对应的文本
};
