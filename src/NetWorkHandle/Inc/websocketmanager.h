//
// Created by Administrator on 2025/2/4.
//
#pragma once
#include <QWebSocket>
#include <QObject>
#include <QThread>
#include <QJsonDocument>
#include <QTimer>
#include <QQueue>
#include <QAtomicPointer>
#include <QMutexLocker>

/**
 * 2025.12.25重构 Misaki
 * 多线程websocket实现
 */
class WebSocketManager final : public QObject
{
Q_OBJECT
public:
    // 构造函数
    explicit WebSocketManager(QObject *parent = nullptr);   // 不携带URL参数
    ~WebSocketManager() override;
    // 删除拷贝构造函数和赋值操作符
    WebSocketManager(const WebSocketManager&) = delete;
    WebSocketManager& operator=(const WebSocketManager&) = delete;
public:
    bool setRequestContent(const QString& requestToken);    // 设置自定义websocket首次请求Token(鉴权用) 服务端应保持Token一致
    bool setSocketUrl(QUrl url);                            // 设置URL

signals:
    // 发给主线程的信号
    void connected();       // 连接
    void disconnected();    // 断开
    void textReceived(const QString &message);      // 接收文本
    void jsonReceived(const QString &type, const QJsonObject &data);    // 接收JSON
    void binaryReceived(const QByteArray &data);    // 接收二进制数据
    void error(const QString &errorMsg);            // 错误
    void log(const QString &msg);                   // 日志
    void reconnecting(int attempt);                 // 重连

public slots:
    // 主线程调用的槽
    bool connectToServer();         // 连接到服务器
    void disconnectFromServer();    // 断开连接
    void sendText(const QString &message);  // 发送文本
    void sendJson(const QString &type, const QJsonObject &data);    // 发送JSON
    void sendBinary(const QByteArray &data);    // 发送二进制数据
    void setReconnectEnabled(bool enabled);         // 设置重连
    void setRequestEnabled(bool enabled);           // 设置自定义首次请求

    [[nodiscard]] bool isConnected() const;         // 是否已连接

private slots:
    void onConnected();     // 连接成功
    void onDisconnected();  // 断开连接
    void onTextMessageReceived(const QString &message);         // 接收到文本消息
    void onError(QAbstractSocket::SocketError socketError);     // 错误
    void onSslErrors(const QList<QSslError> &errors);           // SSL错误
    void onPong(quint64 elapsedTime, const QByteArray &payload);    // Pong
    void sendPing() const;        // 发送Ping
    void tryReconnect();    // 尝试重连

private:
    QWebSocket *m_socket;           /// WebSocket对象
    QUrl m_url;                     /// 服务器地址
    QTimer *m_pingTimer;            /// Ping定时器
    QTimer *m_reconnectTimer;       /// 重连定时器
    int m_reconnectAttempts;        /// 重连尝试次数
    bool m_isReconnectEnabled;      /// 是否启用重连
    QNetworkRequest m_request;      /// 自定义websocket首次请求(鉴权用)
    bool m_isRequest;               /// 是否启用websocket首次请求
};

/**
 * WebSocket 客户端单例管理类
 * 负责管理 WebSocket 线程和全局访问
 */
class WebSocketClient final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(WebSocketClient)

public:
    static WebSocketClient* getInstance();
    static void destroy();

    // 初始化/重新配置 WebSocket
    bool setConfiguration(const QUrl& url, const QString& authToken = QString());

    // WebSocket 操作
    void connectToServer();
    void disconnectFromServer();
    void reconnect();

    void sendText(const QString& message);
    void sendJson(const QString& type, const QJsonObject& data);
    void sendBinary(const QByteArray& data);

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool hasConfiguration() const { return m_url.isValid(); }

    void setAutoReconnect(bool enabled);
    void setPingInterval(int milliseconds);

    [[nodiscard]] QUrl currentUrl() const { return m_url; }
    [[nodiscard]] QString currentToken() const { return m_authToken; }

    // 获取内部管理器（仅供高级使用）
    [[nodiscard]] WebSocketManager* manager() const { return m_webSocketManager; }

signals:
    // WebSocket 事件
    void connected();
    void disconnected();
    void textReceived(const QString &message);
    void jsonReceived(const QString &type, const QJsonObject &data);
    void binaryReceived(const QByteArray &data);
    void error(const QString &errorMsg);
    void log(const QString &msg);
    void reconnecting(int attempt);

    // 配置变更
    void configurationChanged(const QUrl& oldUrl, const QUrl& newUrl);

    // 内部信号（用于跨线程通信）
    void internalSetUrl(const QUrl& url);
    void internalSetAuthToken(const QString& token);
    void internalConnect();
    void internalDisconnect();
    void internalSendText(const QString& message);
    void internalSendJson(const QString& type, const QJsonObject& data);
    void internalSendBinary(const QByteArray& data);
    void internalSetAutoReconnect(bool enabled);
    void internalSetRequestEnabled(bool enabled);
public:
    ~WebSocketClient() override;
private:
    explicit WebSocketClient(QObject *parent = nullptr);

    static QMutex m_mutex;
    static QScopedPointer<WebSocketClient> m_instance;

    QThread* m_workerThread;
    WebSocketManager* m_webSocketManager;
    QUrl m_url;
    QString m_authToken;
    bool m_hasAuthToken;
};