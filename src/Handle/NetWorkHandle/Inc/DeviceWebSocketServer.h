//
// Created by misaki on 2026/4/25.
//

#pragma once
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QHash>
#include <QJsonObject>

class DeviceWebSocketServer final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DeviceWebSocketServer)

public:
    explicit DeviceWebSocketServer(quint16 port = 10002, QObject *parent = nullptr);
    ~DeviceWebSocketServer() override;

    bool start();
    void stop();
    [[nodiscard]] bool isListening() const { return m_server && m_server->isListening(); }
    [[nodiscard]] quint16 serverPort() const { return m_port; }

    Q_INVOKABLE void sendToDevice(const QString &deviceId, const QString &type, const QJsonObject &data);

signals:
    void deviceConnected(const QString &deviceId, const QString &deviceName);
    void deviceDisconnected(const QString &deviceId);
    void jsonReceived(const QString &deviceId, const QString &type, const QJsonObject &data);
    void serverError(const QString &errorMsg);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onTextMessageReceived(const QString &message);

private:
    struct DeviceSession {
        QString deviceId;
        QString deviceName;
        QWebSocket *socket;
    };

    QWebSocketServer *m_server;
    quint16 m_port;
    QHash<QString, DeviceSession*> m_deviceSessions;
    QHash<QWebSocket*, DeviceSession*> m_socketSessions;

    void removeSession(QWebSocket *socket);
};
