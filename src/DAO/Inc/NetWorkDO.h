//
// Created by misaki on 2025/12/29.
//
#pragma once

/**
 * 本类为网络数据流会使用到的数据访问对象封装
 * 目的是便于统一数据访问方式
 * 同时屏蔽了端到端数据交换格式，使得上层调用不再需要关心数据格式，而只需要填入数据即可
 */

/**
 * 简单描述一下Yosuga客户端所需要使用到的数据
 * 主要为音频数据，控制信息，文本信息。
 * 其中文本信息与音频数据为捆绑收发，并且其中还包括了一些特别的信息，例如音频时长等
 * 控制信息与各种业务逻辑相关，例如模拟点击，模拟输入等
 */

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QScopedPointer>
#include <QMutex>
#include <functional>

/**
 * 数据传输对象 (DTO) 定义
 */
// 音频文本捆绑数据结构
struct AudioDataPacket {
    QString text;           // 文本内容
    QByteArray audioData;   // 音频原始数据 (二进制)
    int sampleRate;         // 采样率
    int channels;           // 通道数
    qint64 duration;        // 时长 (ms)

    AudioDataPacket() : sampleRate(16000), channels(1), duration(0) {}
};

//  控制指令数据结构 (预留)
enum class ControlType {
    Click,
    Input,
    Scroll
};

struct ControlDataPacket {
    ControlType action;
    int x;
    int y;
    QString extraData;
};

/**
 * NetworkDO
 */
class NetworkDO final : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(NetworkDO) // 禁用拷贝

public:
    // 单例访问点
    static NetworkDO* getInstance();
    // 显式销毁
    static void destroy();

    // 定义发送回调函数类型
    using SenderFunc = std::function<void(const QString& type, const QJsonObject& data)>;

public:
    // 注入发送接口
    void registerSender(SenderFunc sender);

    // 业务发送函数
    void sendAudioPacket(const AudioDataPacket& packet);
    void sendControlPacket(const ControlDataPacket& packet);

signals:
    // 业务接收信号
    void audioPacketReceived(const AudioDataPacket& packet);            // 音频数据准备完成信号
    void controlPacketReceived(const ControlDataPacket& packet);        // 控制数据准备完成信号
    void errorOccurred(const QString& errorMsg);                        // 错误信号

public slots:
    // 接收底层 JSON 数据
    void onDataReceived(const QString& type, const QJsonObject& data);
public:
    ~NetworkDO() override;
private:
    // 构造/析构函数私有化
    explicit NetworkDO(QObject *parent = nullptr);

    // 内部处理逻辑
    void handleAudioMessage(const QJsonObject& data);

    static QScopedPointer<NetworkDO> m_instance;
    static QMutex m_mutex;

    SenderFunc m_sender; // 注入的发送器
};