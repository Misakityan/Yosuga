//
// Created by misaki on 2026/1/13.
//
#include "AudioDataTransferObject.h"
#include <QJsonValue>
// 构造函数实现（初始化列表）
AudioDataTransferObject::AudioDataTransferObject(const QString& owner,
                                               bool isStream,
                                               bool isStart,
                                               bool isEnd,
                                               int sequence,
                                               const QByteArray& data,
                                               int sampleRate,
                                               int channelCount,
                                               int bitDepth,
                                               double duration,
                                               const QString& text)
    : m_owner(owner)
    , m_isStream(isStream)
    , m_isStart(isStart)
    , m_isEnd(isEnd)
    , m_sequence(sequence)
    , m_data(data)
    , m_sampleRate(sampleRate)
    , m_channelCount(channelCount)
    , m_bitDepth(bitDepth)
    , m_duration(duration)
    , m_text(text) {
}

// 静态工厂方法：从 JSON 反序列化
AudioDataTransferObject AudioDataTransferObject::fromJson(const QJsonObject& json) {
    // 逐个字段读取，不存在则用默认值
    QString owner = json.value("Owner").toString("server");
    bool isStream = json.value("isStream").toBool(false);
    bool isStart = json.value("isStart").toBool(false);
    bool isEnd = json.value("isEnd").toBool(false);
    int sequence = json.value("sequence").toInt(0);
    // 处理 base64 编码的 data 字段
    QByteArray data;
    if (json.contains("data")) {
        const QString base64Str = json.value("data").toString();
        data = QByteArray::fromBase64(base64Str.toUtf8());
    }
    int sampleRate = json.value("sampleRate").toInt(16000);
    int channelCount = json.value("channelCount").toInt(1);
    int bitDepth = json.value("bitDepth").toInt(16);
    double duration = json.value("duration").toDouble(0.0);
    QString text = json.value("text").toString();

    // 调用构造函数创建对象
    return AudioDataTransferObject(owner, isStream, isStart, isEnd,
                                 sequence, data, sampleRate, channelCount,
                                 bitDepth, duration, text);
}

// 序列化为 JSON
QJsonObject AudioDataTransferObject::toJson() const {
    QJsonObject json;
    json["Owner"] = m_owner;
    json["isStream"] = m_isStream;
    json["isStart"] = m_isStart;
    json["isEnd"] = m_isEnd;
    json["sequence"] = m_sequence;
    // data 字段 base64 编码
    json["data"] = QString(m_data.toBase64());
    json["sampleRate"] = m_sampleRate;
    json["channelCount"] = m_channelCount;
    json["bitDepth"] = m_bitDepth;
    json["duration"] = m_duration;
    json["text"] = m_text;
    return json;
}

// 链式设置
AudioDataTransferObject& AudioDataTransferObject::setData(const QString& key,
                                                          const QJsonValue& value) {
    if (key == "Owner") {
        m_owner = value.toString();
    } else if (key == "isStream") {
        m_isStream = value.toBool();
    } else if (key == "isStart") {
        m_isStart = value.toBool();
    } else if (key == "isEnd") {
        m_isEnd = value.toBool();
    } else if (key == "sequence") {
        m_sequence = value.toInt();
    } else if (key == "data") {
        // 这里要求已是 base64 字符串
        m_data = QByteArray::fromBase64(value.toString().toUtf8());
    } else if (key == "sampleRate") {
        m_sampleRate = value.toInt();
    } else if (key == "channelCount") {
        m_channelCount = value.toInt();
    } else if (key == "bitDepth") {
        m_bitDepth = value.toInt();
    } else if (key == "duration") {
        m_duration = value.toDouble();
    } else if (key == "text") {
        m_text = value.toString();
    }
    // 如果 key 不存在，默认忽略
    return *this;  // 返回自身引用，支持链式调用
}