//
// Created by misaki on 2026/1/26.
//

#include "serialportmanager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QJsonDocument>
#include <utility>
#include "cobs.hpp"

/// SerialPortManager
SerialPortManager::SerialPortManager(QString deviceName, QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
    , m_deviceName(std::move(deviceName))
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_isAutoReconnect(false)
    , m_reconnectAttempts(0)
    , m_cobsBuffer()
    , m_cobsInFrame(false)
{
    // 心跳定时器配置（默认 5 秒）
    m_heartbeatTimer->setInterval(5000);
    connect(m_heartbeatTimer, &QTimer::timeout,
            this, &SerialPortManager::sendHeartbeat);

    // 重连定时器（单次触发）
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &SerialPortManager::tryReconnect);

    // 串口信号连接
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialPortManager::onReadyRead);
    connect(m_serial, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &SerialPortManager::onErrorOccurred);

    emit log(QString("[%1] SerialPortManager initialized in worker thread").arg(m_deviceName));
}

SerialPortManager::~SerialPortManager() {
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

bool SerialPortManager::setConfig(const SerialPortConfig &config) {
    if (m_serial->isOpen()) {
        emit error(QString("[%1] Cannot change config while port is open. Close it first.").arg(m_deviceName));
        return false;
    }

    if (config.portName.isEmpty()) {
        emit error(QString("[%1] Port name cannot be empty!").arg(m_deviceName));
        return false;
    }

    m_config = config;
    emit log(QString("[%1] Config updated: %2 @ %3 bps, JSON max: %4 bytes")
                 .arg(m_deviceName, config.portName).arg(config.baudRate).arg(config.maxJsonSize));
    return true;
}

SerialPortManager::SerialPortConfig SerialPortManager::currentConfig() const {
    return m_config;
}

bool SerialPortManager::open() {
    if (m_serial->isOpen()) {
        emit log(QString("[%1] Port already opened, closing first...").arg(m_deviceName));
        m_serial->close();
    }
    // 配置串口参数
    m_serial->setPortName(m_config.portName);
    m_serial->setBaudRate(m_config.baudRate);
    m_serial->setDataBits(m_config.dataBits);
    m_serial->setParity(m_config.parity);
    m_serial->setStopBits(m_config.stopBits);
    m_serial->setFlowControl(m_config.flowControl);

    emit log(QString("[%1] Opening %2...").arg(m_deviceName, m_config.portName));

    if (!m_serial->open(QIODevice::ReadWrite)) {
        QString errMsg = QString("[%1] Failed to open %2: %3")
                             .arg(m_deviceName, m_config.portName, m_serial->errorString());
        emit error(errMsg);
        if (m_isAutoReconnect) {
            m_reconnectTimer->start(3000);
        }
        return false;
    }
    m_reconnectAttempts = 0;
    m_cobsBuffer.clear();   // 清空 COBS 缓冲区
    m_cobsInFrame = false;  // 重置帧状态
    emit opened();
    emit log(QString("[%1] Serial port opened successfully").arg(m_deviceName));

    // 启动心跳（如果有配置心跳包）
    if (!m_config.heartbeatData.isEmpty()) {
        m_heartbeatTimer->start();
    }
    return true;
}

void SerialPortManager::close() {
    m_isAutoReconnect = false;
    m_heartbeatTimer->stop();
    m_reconnectTimer->stop();
    if (m_serial->isOpen()) {
        m_serial->close();
        m_cobsBuffer.clear();      // 清空 COBS 缓冲区
        m_cobsInFrame = false;     // 重置帧状态
        emit closed();
        emit log(QString("[%1] Serial port closed").arg(m_deviceName));
    }
}

void SerialPortManager::sendJson(const QString &type, const QJsonObject &data) {
    if (!m_serial->isOpen()) {
        emit error(QString("[%1] Cannot send JSON: serial port not opened").arg(m_deviceName));
        return;
    }

    // 封装成 { "type": "...", "data": {...}, "timestamp": 123456 }
    QJsonObject wrapper;
    wrapper["type"] = type;
    wrapper["data"] = data;
    wrapper["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(wrapper);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    // COBS 编码
    std::vector<uint8_t> encoded;
    auto result = cobs::encode(encoded, std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(jsonBytes.constData()), jsonBytes.size()
    ));
    if (result.status != cobs::Status::OK) {    // 编码失败
        emit error(QString("[%1] COBS encode failed: status=%2").arg(m_deviceName).arg(static_cast<int>(result.status)));
        return;
    }

    // 发送编码数据 + 0x00
    const qint64 written = m_serial->write(reinterpret_cast<const char*>(encoded.data()), static_cast<qint64>(encoded.size()));
    if (written == -1) {    // 写入失败
        emit error(QString("[%1] Write error: %2").arg(m_deviceName, m_serial->errorString()));
    } else {    // 发送成功
        m_serial->flush();
        m_serial->write("\0", 1); // 帧结束符
        emit log(QString("[%1] Sent JSON: type=%2, %3 bytes encoded").arg(m_deviceName, type).arg(encoded.size() + 1));
    }
}

void SerialPortManager::sendText(const QString &text) {
    if (!m_serial->isOpen()) {
        emit error(QString("[%1] Cannot send text: serial port not opened").arg(m_deviceName));
        return;
    }

    QByteArray data = text.toUtf8();
    qint64 written = m_serial->write(data);
    if (written == -1) {
        emit error(QString("[%1] Write error: %2").arg(m_deviceName, m_serial->errorString()));
    } else if (written != data.size()) {
        emit error(QString("[%1] Incomplete write: %2/%3 bytes sent").arg(m_deviceName).arg(written).arg(data.size()));
    } else {
        m_serial->flush();
        emit log(QString("[%1] Sent text: %2 bytes").arg(m_deviceName).arg(written));
    }
}

void SerialPortManager::sendHex(const QString &hex) {
    if (!m_serial->isOpen()) {
        emit error(QString("[%1] Cannot send hex: serial port not opened").arg(m_deviceName));
        return;
    }

    // 解析十六进制字符串: "AA BB 1A" → QByteArray
    QString cleaned = hex.simplified().remove(' ');
    QByteArray data = QByteArray::fromHex(cleaned.toUtf8());

    if (data.isEmpty() && !cleaned.isEmpty()) {
        emit error(QString("[%1] Invalid hex format. Use: 'AA BB CC' or 'AABBCC'").arg(m_deviceName));
        return;
    }

    qint64 written = m_serial->write(data);
    if (written == -1) {
        emit error(QString("[%1] Write error: %2").arg(m_deviceName, m_serial->errorString()));
    } else {
        m_serial->flush();
        emit log(QString("[%1] Sent hex: %2").arg(m_deviceName, cleaned.left(50)));
    }
}

void SerialPortManager::sendRaw(const QByteArray &data) {
    if (!m_serial->isOpen()) {
        emit error(QString("[%1] Cannot send raw: serial port not opened").arg(m_deviceName));
        return;
    }

    qint64 written = m_serial->write(data);
    if (written == -1) {
        emit error(QString("[%1] Write error: %2").arg(m_deviceName, m_serial->errorString()));
    } else {
        m_serial->flush();
        emit log(QString("[%1] Sent raw: %2 bytes").arg(m_deviceName).arg(written));
    }
}

bool SerialPortManager::isOpen() const {
    return m_serial->isOpen();
}

void SerialPortManager::setAutoReconnect(bool enabled) {
    m_isAutoReconnect = enabled;
    if (!enabled) {
        m_reconnectTimer->stop();
    }
    emit log(QString("[%1] Auto reconnect %2").arg(m_deviceName, enabled ? "enabled" : "disabled"));
}

void SerialPortManager::setHeartbeatInterval(int msecs) {
    m_heartbeatTimer->setInterval(msecs);
    emit log(QString("[%1] Heartbeat interval: %2 ms").arg(m_deviceName).arg(msecs));
}

void SerialPortManager::onReadyRead() {
    // 读取所有可用数据到 COBS 缓冲区
    QByteArray chunk = m_serial->readAll();     // 读取原始字节流
    m_cobsBuffer.append(chunk);                 // 追加到cobs缓冲区

    // 触发原始数据信号
    emit dataReceived(chunk);
    emit textReceived(QString::fromUtf8(chunk)); // 尝试 UTF-8 解码
    emit hexReceived(chunk.toHex(' ').toUpper()); // 十六进制表示

    // 处理 COBS 帧
    processCOBSBuffer();
}

void SerialPortManager::processCOBSBuffer() {
    while (true) {
        // 查找帧结束符 0x00
        int zeroPos = m_cobsBuffer.indexOf('\0');
        // 未找到完整帧，继续等待
        if (zeroPos == -1) {
            m_cobsInFrame = true;
            // 防溢出
            if (m_cobsBuffer.size() > m_config.maxJsonSize * 2) {
                emit error(QString("[%1] COBS buffer overflow, clearing").arg(m_deviceName));
                m_cobsBuffer.clear();
                m_cobsInFrame = false;
            }
            return;
        }
        // 提取编码帧（不含 0x00）
        QByteArray encodedFrame = m_cobsBuffer.left(zeroPos);
        m_cobsBuffer.remove(0, zeroPos + 1); // 移除结束符
        m_cobsInFrame = false;
        // 跳过空帧
        if (encodedFrame.isEmpty()) {
            emit log(QString("[%1] Empty COBS frame, skipped").arg(m_deviceName));
            continue;
        }
        // COBS 解码
        std::vector<uint8_t> decoded;
        auto result = cobs::decode(decoded, std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(encodedFrame.constData()), encodedFrame.size()
        ));

        if (result.status != cobs::Status::OK) {    // 解码失败
            emit error(QString("[%1] COBS decode failed: status=%2").arg(m_deviceName).arg(static_cast<int>(result.status)));
            continue;
        }
        // 解析 JSON
        QByteArray jsonData(reinterpret_cast<const char*>(decoded.data()), static_cast<qint64>(decoded.size()));
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit error(QString("[%1] JSON parse error: %2").arg(m_deviceName, parseError.errorString()));
            continue;
        }
        if (!doc.isObject()) {
            emit error(QString("[%1] JSON is not an object").arg(m_deviceName));
            continue;
        }

        QJsonObject obj = doc.object();
        const QString type = obj.value("type").toString();
        const QJsonObject data = obj.value("data").toObject();

        if (type.isEmpty()) {
            emit error(QString("[%1] JSON missing 'type' field").arg(m_deviceName));
            continue;
        }

        // 成功，向上层交付
        emit jsonReceived(type, data);
        emit log(QString("[%1] JSON delivered: type=%2, size=%3").arg(m_deviceName, type).arg(jsonData.size()));
    }
}

void SerialPortManager::onErrorOccurred(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError) return;

    QString errorMsg;
    switch (error) {
        case QSerialPort::DeviceNotFoundError:
            errorMsg = "Device not found";
            break;
        case QSerialPort::PermissionError:
            errorMsg = "Permission denied. Check udev rules or run with sudo";
            break;
        case QSerialPort::OpenError:
            errorMsg = "Already opened or system error";
            break;
        case QSerialPort::WriteError:
            errorMsg = "Write error";
            break;
        case QSerialPort::ReadError:
            errorMsg = "Read error";
            break;
        case QSerialPort::ResourceError:
            errorMsg = "Resource error: device removed or I/O error";
            // 设备被拔插，触发重连
            if (m_isAutoReconnect) {
                m_reconnectTimer->start(2000);
            }
            break;
        case QSerialPort::UnsupportedOperationError:
            errorMsg = "Unsupported operation";
            break;
        case QSerialPort::TimeoutError:
            errorMsg = "Operation timed out";
            break;
        case QSerialPort::NotOpenError:
            errorMsg = "Device not open";
            break;
        default:
            errorMsg = m_serial->errorString();
    }

    emit this->error(QString("[%1] Serial error: %2").arg(m_deviceName, errorMsg));
}

void SerialPortManager::sendHeartbeat() {
    if (!m_serial->isOpen() || m_config.heartbeatData.isEmpty()) {
        return;
    }

    qint64 written = m_serial->write(m_config.heartbeatData);
    if (written == -1) {
        emit error(QString("[%1] Heartbeat write failed: %2").arg(m_deviceName, m_serial->errorString()));
    } else {
        emit log(QString("[%1] Heartbeat sent").arg(m_deviceName));
    }
}

void SerialPortManager::tryReconnect() {
    if (!m_isAutoReconnect) return;

    m_reconnectAttempts++;
    emit reconnecting(m_reconnectAttempts);
    emit log(QString("[%1] Reconnecting... (attempt %2)").arg(m_deviceName).arg(m_reconnectAttempts));

    // 直接调用 open()
    open();
}

/// SerialPortClient
SerialPortClient::SerialPortClient(const QString &deviceName, QObject *parent)
    : QObject(parent)
    , m_deviceName(deviceName)
    , m_workerThread(new QThread(this))
    , m_serialManager(new SerialPortManager(deviceName))
    , m_config()
{
    // 命名线程，方便调试
    m_workerThread->setObjectName(QString("SerialPortThread_%1").arg(deviceName));

    // 将 Manager 移到工作线程
    m_serialManager->moveToThread(m_workerThread);

    // 线程结束时清理 Manager
    connect(m_workerThread, &QThread::finished,
            m_serialManager, &QObject::deleteLater);

    // 信号转发：Manager → Client（主线程）
    connect(m_serialManager, &SerialPortManager::opened,
            this, &SerialPortClient::opened);
    connect(m_serialManager, &SerialPortManager::closed,
            this, &SerialPortClient::closed);
    connect(m_serialManager, &SerialPortManager::dataReceived,
            this, &SerialPortClient::dataReceived);
    connect(m_serialManager, &SerialPortManager::textReceived,
            this, &SerialPortClient::textReceived);
    connect(m_serialManager, &SerialPortManager::hexReceived,
            this, &SerialPortClient::hexReceived);
    connect(m_serialManager, &SerialPortManager::jsonReceived,
            this, &SerialPortClient::jsonReceived);
    connect(m_serialManager, &SerialPortManager::error,
            this, &SerialPortClient::error);
    connect(m_serialManager, &SerialPortManager::log,
            this, &SerialPortClient::log);
    connect(m_serialManager, &SerialPortManager::reconnecting,
            this, &SerialPortClient::reconnecting);

    // 内部信号：Client → Manager（跨线程调用）
    connect(this, &SerialPortClient::internalSetConfig,
            m_serialManager, [this](const SerialPortManager::SerialPortConfig& cfg) {
                bool ok = m_serialManager->setConfig(cfg);
                if (!ok) emit error(QString("[%1] Failed to set config").arg(m_deviceName));
            }, Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalOpen,
            m_serialManager, &SerialPortManager::open,
            Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalClose,
            m_serialManager, &SerialPortManager::close,
            Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalSendJson,
            m_serialManager, &SerialPortManager::sendJson,
            Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalSendText,
            m_serialManager, &SerialPortManager::sendText,
            Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalSendHex,
            m_serialManager, &SerialPortManager::sendHex,
            Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalSendRaw,
            m_serialManager, &SerialPortManager::sendRaw,
            Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalSetAutoReconnect,
            m_serialManager, &SerialPortManager::setAutoReconnect,
            Qt::QueuedConnection);

    connect(this, &SerialPortClient::internalSetHeartbeatInterval,
            m_serialManager, &SerialPortManager::setHeartbeatInterval,
            Qt::QueuedConnection);

    // 启动工作线程
    m_workerThread->start();

    emit log(QString("[%1] SerialPortClient initialized, worker thread started").arg(m_deviceName));
}

SerialPortClient::~SerialPortClient() {
    emit log(QString("[%1] Shutting down SerialPortClient...").arg(m_deviceName));

    // 关闭串口
    close();

    // 优雅退出工作线程
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        if (!m_workerThread->wait(3000)) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
        delete m_workerThread;
        m_workerThread = nullptr;
    }
}

bool SerialPortClient::setConfiguration(const SerialPortManager::SerialPortConfig &config) {
    if (config.portName.isEmpty()) {
        emit error(QString("[%1] Invalid config: port name empty").arg(m_deviceName));
        return false;
    }

    auto oldConfig = m_config;
    m_config = config;

    // 跨线程设置
    emit internalSetConfig(config);

    // 通知配置变更
    if (oldConfig.portName != config.portName || oldConfig.baudRate != config.baudRate) {
        emit configurationChanged(oldConfig, config);
    }

    emit log(QString("[%1] Configuration updated: %2 @ %3 bps")
                 .arg(m_deviceName, config.portName).arg(config.baudRate));
    return true;
}

void SerialPortClient::open() {
    if (!hasConfiguration()) {
        emit error(QString("[%1] Not configured. Call setConfiguration() first.").arg(m_deviceName));
        return;
    }
    emit log(QString("[%1] Opening serial port: %2").arg(m_deviceName, m_config.portName));
    emit internalOpen();
}

void SerialPortClient::close() {
    emit log(QString("[%1] Closing serial port...").arg(m_deviceName));
    emit internalClose();
}

void SerialPortClient::reconnect() {
    if (!hasConfiguration()) {
        emit error(QString("[%1] No configuration. Cannot reconnect.").arg(m_deviceName));
        return;
    }
    emit log(QString("[%1] Attempting to reconnect...").arg(m_deviceName));
    close();
    QTimer::singleShot(100, this, [this]() { open(); });
}

void SerialPortClient::sendJson(const QString &type, const QJsonObject &data) {
    if (!isOpen()) {
        emit error(QString("[%1] Cannot send JSON: serial port not opened").arg(m_deviceName));
        return;
    }
    emit internalSendJson(type, data);
}

void SerialPortClient::sendText(const QString &text) {
    if (!isOpen()) {
        emit error(QString("[%1] Cannot send text: serial port not opened").arg(m_deviceName));
        return;
    }
    emit internalSendText(text);
}

void SerialPortClient::sendHex(const QString &hex) {
    if (!isOpen()) {
        emit error(QString("[%1] Cannot send hex: serial port not opened").arg(m_deviceName));
        return;
    }
    emit internalSendHex(hex);
}

void SerialPortClient::sendRaw(const QByteArray &data) {
    if (!isOpen()) {
        emit error(QString("[%1] Cannot send raw: serial port not opened").arg(m_deviceName));
        return;
    }
    emit internalSendRaw(data);
}

bool SerialPortClient::isOpen() const {
    if (m_serialManager) {
        bool opened = false;
        QMetaObject::invokeMethod(const_cast<SerialPortManager*>(m_serialManager),
                                  [&opened, mgr = m_serialManager]() {
                                      opened = mgr->isOpen();
                                  },
                                  Qt::BlockingQueuedConnection);
        return opened;
    }
    return false;
}

void SerialPortClient::setAutoReconnect(bool enabled) {
    emit log(QString("[%1] Auto reconnect %2").arg(m_deviceName, enabled ? "enabled" : "disabled"));
    emit internalSetAutoReconnect(enabled);
}

void SerialPortClient::setHeartbeatInterval(int msecs) {
    emit log(QString("[%1] Heartbeat interval: %2 ms").arg(m_deviceName).arg(msecs));
    emit internalSetHeartbeatInterval(msecs);
}

QStringList SerialPortClient::availablePorts() {
    QStringList ports;
    const auto portList = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : portList) {
        QString desc = QString("%1 (%2)").arg(info.portName(), info.description());
        ports << desc;
    }
    return ports;
}