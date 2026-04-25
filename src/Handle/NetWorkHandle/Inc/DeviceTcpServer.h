//
// Created by misaki on 2026/4/25.
//

#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>

class DeviceTcpServer final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DeviceTcpServer)

public:
    explicit DeviceTcpServer(quint16 port = 10001, QObject *parent = nullptr);
    ~DeviceTcpServer() override;

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
    void onReadyRead();

private:
    struct DeviceSession {
        QString deviceId;
        QString deviceName;
        QTcpSocket *socket;
        QByteArray buffer;
    };

    QTcpServer *m_server;
    quint16 m_port;
    QHash<QString, DeviceSession*> m_deviceSessions;  // deviceId -> session
    QHash<QTcpSocket*, DeviceSession*> m_socketSessions; // socket -> session

    void parseIncomingData(DeviceSession *session);
    void removeSession(QTcpSocket *socket);
};
