//
// Created by Administrator on 2025/2/4.
//

#include "websocketmanager.h"
#include <QDebug>
#include <QJsonObject>
#include <utility>
#include <QMutexLocker>

/// WebSocketManager
WebSocketManager::WebSocketManager(QObject *parent)
    : QObject(parent)
    , m_socket(new QWebSocket)      // 创建 WebSocket 对象
    , m_pingTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_reconnectAttempts(0)        // 重连尝试次数初始为0
    , m_isReconnectEnabled(false)   // 默认不启用重连
    , m_isRequest(false)            // 默认不启用自定义首次请求
{
    // 配置 WebSocket
    m_socket->setParent(this); // 确保 socket 也在工作线程
    m_socket->setMaxAllowedIncomingFrameSize(50 * 1024 * 1024);     // 单帧最大 50MB        // 防止发送大数据包导致websocket断开
    m_socket->setMaxAllowedIncomingMessageSize(50 * 1024 * 1024);   // 完整消息最大 50MB
    // 连接信号
    connect(m_socket, &QWebSocket::connected,
            this, &WebSocketManager::onConnected);
    connect(m_socket, &QWebSocket::disconnected,
            this, &WebSocketManager::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &WebSocketManager::onTextMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &WebSocketManager::onError);
    connect(m_socket, &QWebSocket::sslErrors,
            this, &WebSocketManager::onSslErrors);
    connect(m_socket, &QWebSocket::pong,
            this, &WebSocketManager::onPong);
    // 心跳定时器
    m_pingTimer->setInterval(30000); // 30秒
    connect(m_pingTimer, &QTimer::timeout, this, &WebSocketManager::sendPing);

    // 重连定时器
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketManager::tryReconnect);
}

WebSocketManager::~WebSocketManager() {
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
    }
}
bool WebSocketManager::setRequestContent(const QString& requestToken) {
    if (this->m_url.isEmpty()) {
        emit error("URL is empty!");
        return false;
    }
    m_request.setUrl(this->m_url);
    m_request.setRawHeader("Authorization", requestToken.toUtf8());     // 设置请求头(包含鉴权Token)
    return true;
}

bool WebSocketManager::setSocketUrl(QUrl url) {
    if (this->m_url == url) {return true;}    // 如果 URL 没有变化，直接返回成功
    // 判断URL是否合法，即是否符合websocket的格式
    if (!url.isValid() || url.scheme() != "ws" && url.scheme() != "wss") {
        emit error("Invalid URL!");
        return false;
    }
    this->m_url = std::move(url);   // 设置 URL
    return true;
}

bool WebSocketManager::connectToServer() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {   // 如果已经连接，则先断开连接重新连接
        m_socket->close();
    }
    if (m_url.isEmpty()) {      // 如果 URL 为空
        emit error("URL is empty!");
        return false;
    }
    emit log(QString("Connecting to %1...").arg(m_url.toString()));

    // SSL 配置
    if (m_url.scheme() == "wss") {
        QSslConfiguration sslConfig = m_socket->sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // 开发环境
        m_socket->setSslConfiguration(sslConfig);
    }
    if (m_isRequest) {  // 如果需要自定义首次请求
        m_socket->open(this->m_request);    // 使用重载函数
    }
    else {
        m_socket->open(m_url);
    }
    return true;
}

void WebSocketManager::disconnectFromServer() {
    m_isReconnectEnabled = false; // 手动断开不重连
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close(QWebSocketProtocol::CloseCodeNormal, "Client closed");
    }
}

void WebSocketManager::sendText(const QString &message) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->sendTextMessage(message);
    }
}

void WebSocketManager::sendJson(const QString &type, const QJsonObject &data) {
    QJsonObject wrapper;
    wrapper["type"] = type;
    wrapper["data"] = data;
    wrapper["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    sendText(QJsonDocument(wrapper).toJson(QJsonDocument::Compact));
}

void WebSocketManager::sendBinary(const QByteArray &data) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->sendBinaryMessage(data);
    }
}

bool WebSocketManager::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void WebSocketManager::setReconnectEnabled(bool enabled) {
    m_isReconnectEnabled = enabled;
    if (!enabled) {
        m_reconnectTimer->stop();
    }
}
void WebSocketManager::setRequestEnabled(const bool enabled) {
    m_isRequest = enabled;
}

// 私有槽
void WebSocketManager::onConnected() {
    m_reconnectAttempts = 0;
    emit log("✅ WebSocket connected");
    emit connected();
    m_pingTimer->start();
}

void WebSocketManager::onDisconnected() {
    emit log("⚠️ WebSocket disconnected");
    emit disconnected();
    m_pingTimer->stop();

    // 自动重连
    if (m_isReconnectEnabled) {
        m_reconnectTimer->start(3000); // 3秒后重试
    }
}

void WebSocketManager::onTextMessageReceived(const QString &message) {
    emit textReceived(message);

    // 自动解析 JSON
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        const QString type = obj.value("type").toString();
        const QJsonObject data = obj.value("data").toObject();
        if (!type.isEmpty()) {
            emit jsonReceived(type, data);
        }
    }
}

void WebSocketManager::onError(QAbstractSocket::SocketError socketError) {
    QString errorMsg;
    switch (socketError) {
        case QAbstractSocket::ConnectionRefusedError:
            errorMsg = "Connection refused";
            break;
        case QAbstractSocket::RemoteHostClosedError:
            errorMsg = "Remote host closed";
            break;
        case QAbstractSocket::HostNotFoundError:
            errorMsg = "Host not found";
            break;
        case QAbstractSocket::SocketTimeoutError:
            errorMsg = "Socket timeout";
            break;
        case QAbstractSocket::NetworkError:
            errorMsg = "Network error";
            break;
        case QAbstractSocket::SslHandshakeFailedError:
            errorMsg = "SSL handshake failed";
            break;
        default:
            errorMsg = m_socket->errorString();
    }
    emit error(QString("Socket error: %1").arg(errorMsg));
}

void WebSocketManager::onSslErrors(const QList<QSslError> &errors) {
    foreach (const QSslError &err, errors) {
        emit log(QString("SSL error: %1").arg(err.errorString()));
    }
#ifdef QT_DEBUG
    m_socket->ignoreSslErrors(); // 开发环境忽略
#endif
}

void WebSocketManager::sendPing() const {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->ping();
    }
}

void WebSocketManager::onPong(quint64 elapsedTime, const QByteArray &) {
    emit log(QString("Pong received, latency: %1ms").arg(elapsedTime));
}

void WebSocketManager::tryReconnect() {
    if (!m_isReconnectEnabled) return;

    m_reconnectAttempts++;
    emit reconnecting(m_reconnectAttempts);
    emit log(QString("🔄 Reconnecting... (attempt %1)").arg(m_reconnectAttempts));

    m_socket->open(m_url);
}

#include <QScopedPointer>
QMutex WebSocketClient::m_mutex;
QScopedPointer<WebSocketClient> WebSocketClient::m_instance;

WebSocketClient* WebSocketClient::getInstance()
{
    QMutexLocker locker(&m_mutex);
    if (m_instance.isNull()) {
        m_instance.reset(new WebSocketClient());
    }
    return m_instance.data();
}

void WebSocketClient::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset();
    }
}

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_workerThread(nullptr)
    , m_webSocketManager(nullptr)
    , m_hasAuthToken(false)
{
    // 创建工作线程
    m_workerThread = new QThread(this);
    m_workerThread->setObjectName("WebSocketWorkerThread");

    // 创建 WebSocketManager
    m_webSocketManager = new WebSocketManager();
    m_webSocketManager->moveToThread(m_workerThread);

    // 连接线程结束信号
    connect(m_workerThread, &QThread::finished,
            m_webSocketManager, &QObject::deleteLater);

    // 连接 WebSocketManager 的信号到本类的信号（转发到主线程）
    connect(m_webSocketManager, &WebSocketManager::connected,
            this, &WebSocketClient::connected);
    connect(m_webSocketManager, &WebSocketManager::disconnected,
            this, &WebSocketClient::disconnected);
    connect(m_webSocketManager, &WebSocketManager::textReceived,
            this, &WebSocketClient::textReceived);
    connect(m_webSocketManager, &WebSocketManager::jsonReceived,
            this, &WebSocketClient::jsonReceived);
    connect(m_webSocketManager, &WebSocketManager::binaryReceived,
            this, &WebSocketClient::binaryReceived);
    connect(m_webSocketManager, &WebSocketManager::error,
            this, &WebSocketClient::error);
    connect(m_webSocketManager, &WebSocketManager::log,
            this, &WebSocketClient::log);
    connect(m_webSocketManager, &WebSocketManager::reconnecting,
            this, &WebSocketClient::reconnecting);

    // 连接本类的内部信号到 WebSocketManager 的槽（跨线程调用）
    // 注意：由于 internal* 信号是私有信号，我们需要使用 lambda 包装器
    connect(this, &WebSocketClient::internalSetUrl,
            m_webSocketManager, [this](const QUrl& url) {
                bool success = m_webSocketManager->setSocketUrl(url);
                if (!success) {
                    emit error("Failed to set WebSocket URL");
                }
            }, Qt::QueuedConnection);

    connect(this, &WebSocketClient::internalSetAuthToken,
            m_webSocketManager, [this](const QString& token) {
                if (!token.isEmpty()) {
                    bool success = m_webSocketManager->setRequestContent(token);
                    if (!success) {
                        emit error("Failed to set authentication token");
                    }
                    m_webSocketManager->setRequestEnabled(true);
                } else {
                    m_webSocketManager->setRequestEnabled(false);
                }
            }, Qt::QueuedConnection);

    connect(this, &WebSocketClient::internalConnect,
            m_webSocketManager, &WebSocketManager::connectToServer,
            Qt::QueuedConnection);

    connect(this, &WebSocketClient::internalDisconnect,
            m_webSocketManager, &WebSocketManager::disconnectFromServer,
            Qt::QueuedConnection);

    connect(this, &WebSocketClient::internalSendText,
            m_webSocketManager, &WebSocketManager::sendText,
            Qt::QueuedConnection);

    connect(this, &WebSocketClient::internalSendJson,
            m_webSocketManager, &WebSocketManager::sendJson,
            Qt::QueuedConnection);

    connect(this, &WebSocketClient::internalSendBinary,
            m_webSocketManager, &WebSocketManager::sendBinary,
            Qt::QueuedConnection);

    connect(this, &WebSocketClient::internalSetAutoReconnect,
            m_webSocketManager, &WebSocketManager::setReconnectEnabled,
            Qt::QueuedConnection);

    // 启动工作线程
    m_workerThread->start();

    qDebug() << "WebSocketClient initialized, worker thread:" << m_workerThread;
}

WebSocketClient::~WebSocketClient()
{
    qDebug() << "Shutting down WebSocketClient...";

    // 断开连接
    disconnectFromServer();

    // 停止工作线程
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

bool WebSocketClient::setConfiguration(const QUrl& url, const QString& authToken)
{
    if (!url.isValid()) {
        emit error("Invalid URL provided");
        return false;
    }

    QUrl oldUrl = m_url;
    m_url = url;
    m_authToken = authToken;
    m_hasAuthToken = !authToken.isEmpty();

    // 发送到工作线程进行配置
    emit internalSetUrl(url);
    if (!authToken.isEmpty()) {
        emit internalSetAuthToken(authToken);
    }

    // 通知配置变更
    if (oldUrl != url) {
        emit configurationChanged(oldUrl, url);
    }

    emit log(QString("WebSocket configuration updated: %1").arg(url.toString()));
    return true;
}

void WebSocketClient::connectToServer()
{
    if (!m_url.isValid()) {
        emit error("WebSocket URL not configured. Call setConfiguration() first.");
        return;
    }

    emit log(QString("Connecting to server: %1").arg(m_url.toString()));
    emit internalConnect();
}

void WebSocketClient::disconnectFromServer()
{
    emit log("Disconnecting from server...");
    emit internalDisconnect();
}

void WebSocketClient::reconnect()
{
    if (!hasConfiguration()) {
        emit error("WebSocket not configured. Cannot reconnect.");
        return;
    }

    // 先断开，再连接
    disconnectFromServer();

    // 短暂延迟后重新连接
    QTimer::singleShot(100, this, [this]() {
        emit log("Attempting to reconnect...");
        connectToServer();
    });
}

void WebSocketClient::sendText(const QString& message)
{
    if (isConnected()) {
        emit internalSendText(message);
    } else {
        emit error("Cannot send message: WebSocket is not connected");
    }
}

void WebSocketClient::sendJson(const QString& type, const QJsonObject& data)
{
    if (isConnected()) {
        emit internalSendJson(type, data);
    } else {
        emit error("Cannot send JSON: WebSocket is not connected");
    }
}

void WebSocketClient::sendBinary(const QByteArray& data)
{
    if (isConnected()) {
        emit internalSendBinary(data);
    } else {
        emit error("Cannot send binary data: WebSocket is not connected");
    }
}

bool WebSocketClient::isConnected() const
{
    if (m_webSocketManager) {
        bool connected = false;
        // 使用阻塞调用获取连接状态
        QMetaObject::invokeMethod(const_cast<WebSocketManager*>(m_webSocketManager),
                                  [&connected, manager = m_webSocketManager]() {
                                      connected = manager->isConnected();
                                  },
                                  Qt::BlockingQueuedConnection);
        return connected;
    }
    return false;
}

void WebSocketClient::setAutoReconnect(bool enabled)
{
    emit log(QString("Auto reconnect %1").arg(enabled ? "enabled" : "disabled"));
    emit internalSetAutoReconnect(enabled);
}

void WebSocketClient::setPingInterval(int milliseconds)
{
    // 注意：需要在 WebSocketManager 中添加相应的方法才能支持
    // 这里暂时记录日志，提醒需要实现
    emit log(QString("setPingInterval(%1) called, but not implemented in WebSocketManager")
             .arg(milliseconds));
    Q_UNUSED(milliseconds)
}