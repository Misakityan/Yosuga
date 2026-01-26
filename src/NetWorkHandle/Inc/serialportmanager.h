//
// Created by misaki on 2026/1/26.
//

/**
 * 串口通信模块 —— 支持多设备并行 + JSON 协议
 * 每个 SerialPortClient 实例对应一个物理串口
 */
#pragma once
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QScopedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

/**
 * @brief 串口管理器（工作线程侧）
 * @details 负责单一串口的实际 I/O、参数配置、心跳、JSON 自动编解码
 *          与 WebSocketManager 对称设计，支持多实例并行
 */
class SerialPortManager final : public QObject {
    Q_OBJECT
public:
    explicit SerialPortManager(QString deviceName, QObject *parent = nullptr);
    ~SerialPortManager() override;

    // 禁止拷贝
    SerialPortManager(const SerialPortManager&) = delete;
    SerialPortManager& operator=(const SerialPortManager&) = delete;

    // 设备标识（用于多实例区分）
    [[nodiscard]] QString deviceName() const { return m_deviceName; }

    // 配置结构体
    struct SerialPortConfig {
        QString portName;              // 端口名: "COM3", "/dev/ttyUSB0"
        qint32 baudRate;               // 波特率: 9600, 115200
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBits;
        QSerialPort::FlowControl flowControl;
        QByteArray heartbeatData;      // 心跳包内容（为空则禁用）
        int maxJsonSize;               // JSON 最大长度（防内存溢出）

        // 默认配置：115200-N-8-1 + 无流控 + 心跳禁用 + JSON 上限 64KB
        explicit SerialPortConfig(
            QString port = "",
            qint32 baud = 115200,
            QSerialPort::DataBits db = QSerialPort::Data8,
            QSerialPort::Parity p = QSerialPort::NoParity,
            QSerialPort::StopBits sb = QSerialPort::OneStop,
            QSerialPort::FlowControl fc = QSerialPort::NoFlowControl,
            QByteArray hb = QByteArray(),
            int maxJson = 65536
        ) : portName(std::move(port)), baudRate(baud), dataBits(db), parity(p),
            stopBits(sb), flowControl(fc), heartbeatData(std::move(hb)), maxJsonSize(maxJson) {}
    };

signals:
    // 状态与数据信号
    void opened();                                                // 串口成功打开
    void closed();                                                // 串口关闭
    void dataReceived(const QByteArray &data);                    // 原始二进制数据
    void textReceived(const QString &text);                       // 文本数据（UTF-8 解码）
    void hexReceived(const QString &hex);                         // 十六进制字符串: "AA BB CC"
    void jsonReceived(const QString &type, const QJsonObject &data); // JSON 已解析
    void error(const QString &errorMsg);                          // 错误信息
    void log(const QString &msg);                                 // 运行日志
    void reconnecting(int attempt);                               // 自动重连中

public slots:
    bool setConfig(const SerialPortManager::SerialPortConfig& config);               // 设置配置
    [[nodiscard]] SerialPortManager::SerialPortConfig currentConfig() const;

    bool open();                                                  // 打开串口
    void close();                                                 // 关闭串口

    // 多层次发送接口（JSON、文本、HEX、原始二进制）
    void sendJson(const QString &type, const QJsonObject &data);  // 发送 JSON（自动封装）
    void sendText(const QString &text);                           // 发送文本（UTF-8）
    void sendHex(const QString &hex);                             // 发送十六进制: "12 AB CD"
    void sendRaw(const QByteArray &data);                         // 发送原始二进制（重命名为 sendRaw 更清晰）

    void setAutoReconnect(bool enabled);                          // 是否开启自动重连
    void setHeartbeatInterval(int msecs);                         // 心跳间隔（毫秒）

    [[nodiscard]] bool isOpen() const;                            // 串口是否已打开

private slots:
    void onReadyRead();                                           // 串口有数据到达
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void sendHeartbeat();                                         // 定时发送心跳
    void tryReconnect();                                          // 重连逻辑

    void processCOBSBuffer();                                     // 尝试解析 COBS帧

private:
    QSerialPort *m_serial;                   /// 串口实例
    QString m_deviceName;                    /// 设备标识（如 "STM32_Master", "ESP32_Slave"）
    SerialPortConfig m_config;               /// 当前配置
    QTimer *m_heartbeatTimer;                /// 心跳定时器
    QTimer *m_reconnectTimer;                /// 重连定时器
    bool m_isAutoReconnect;                  /// 是否启用自动重连
    int m_reconnectAttempts;                 /// 重连尝试次数

    QByteArray m_cobsBuffer;                 /// COBS 解码缓冲区
    bool m_cobsInFrame;                      /// 帧状态
};


/**
 * @brief 串口客户端（主线程接口层）
 * @details 每个实例对应一个物理串口，支持构造多个并行工作
 *          封装线程迁移、信号转发、生命周期管理
 */
class SerialPortClient final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(SerialPortClient)

public:
    // 构造函数：deviceName 为设备标识，用于日志和多实例区分
    explicit SerialPortClient(const QString &deviceName, QObject *parent = nullptr);
    ~SerialPortClient() override;

    // 设备标识
    [[nodiscard]] QString deviceName() const { return m_deviceName; }

    // 配置串口
    bool setConfiguration(const SerialPortManager::SerialPortConfig& config);

    // 串口操作
    void open();
    void close();
    void reconnect();

    // 多层次发送接口
    void sendJson(const QString &type, const QJsonObject &data);
    void sendText(const QString &text);
    void sendHex(const QString &hex);
    void sendRaw(const QByteArray &data);

    // 状态查询
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool hasConfiguration() const { return !m_config.portName.isEmpty(); }
    [[nodiscard]] SerialPortManager::SerialPortConfig currentConfig() const { return m_config; }

    // 高级功能
    void setAutoReconnect(bool enabled);
    void setHeartbeatInterval(int msecs);
    [[nodiscard]] static QStringList availablePorts(); // 枚举系统可用串口

signals:
    // 事件信号（与 SerialPortManager 一一对应，转发到主线程）
    void opened();
    void closed();
    void dataReceived(const QByteArray &data);
    void textReceived(const QString &text);
    void hexReceived(const QString &hex);
    void jsonReceived(const QString &type, const QJsonObject &data);
    void error(const QString &errorMsg);
    void log(const QString &msg);
    void reconnecting(int attempt);

    // 配置变更
    void configurationChanged(const SerialPortManager::SerialPortConfig &oldConfig,
                              const SerialPortManager::SerialPortConfig &newConfig);

private:
    // 内部信号（用于跨线程通信，对标 internal*）
    Q_SIGNAL void internalSetConfig(const SerialPortManager::SerialPortConfig& config);
    Q_SIGNAL void internalOpen();
    Q_SIGNAL void internalClose();
    Q_SIGNAL void internalSendJson(const QString &type, const QJsonObject &data);
    Q_SIGNAL void internalSendText(const QString &text);
    Q_SIGNAL void internalSendHex(const QString &hex);
    Q_SIGNAL void internalSendRaw(const QByteArray &data);
    Q_SIGNAL void internalSetAutoReconnect(bool enabled);
    Q_SIGNAL void internalSetHeartbeatInterval(int msecs);

    QString m_deviceName;                  /// 设备标识
    QThread *m_workerThread;               /// 工作线程
    SerialPortManager *m_serialManager;    /// 管理器实例（工作线程侧）
    SerialPortManager::SerialPortConfig m_config; /// 配置缓存
};