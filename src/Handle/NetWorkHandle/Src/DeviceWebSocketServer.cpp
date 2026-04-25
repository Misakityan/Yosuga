//
// Created by misaki on 2026/4/25.
//

#include "DeviceWebSocketServer.h"
#include <QDebug>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

DeviceWebSocketServer::DeviceWebSocketServer(quint16 port, QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_port(port)
{
}

DeviceWebSocketServer::~DeviceWebSocketServer()
{
    stop();
}

bool DeviceWebSocketServer::start()
{
    if (m_server && m_server->isListening()) {
        qDebug() << "[DeviceWsServer] Already listening on port" << m_port;
        return true;
    }

    m_server = new QWebSocketServer("Yosuga-Device-WS", QWebSocketServer::NonSecureMode, this);
    if (!m_server->listen(QHostAddress::Any, m_port)) {
        QString err = QString("Failed to listen on WS port %1: %2")
                          .arg(m_port).arg(m_server->errorString());
        qWarning() << "[DeviceWsServer]" << err;
        emit serverError(err);
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection,
            this, &DeviceWebSocketServer::onNewConnection);

    qDebug() << "[DeviceWsServer] Listening for embedded devices on WS port" << m_port;
    return true;
}

void DeviceWebSocketServer::stop()
{
    for (auto it = m_socketSessions.begin(); it != m_socketSessions.end(); ++it) {
        DeviceSession *session = it.value();
        if (session->socket->state() == QAbstractSocket::ConnectedState) {
            session->socket->close();
        }
        delete session;
    }
    m_deviceSessions.clear();
    m_socketSessions.clear();

    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}

void DeviceWebSocketServer::sendToDevice(const QString &deviceId, const QString &type, const QJsonObject &data)
{
    DeviceSession *session = m_deviceSessions.value(deviceId);
    if (!session || !session->socket) {
        qWarning() << "[DeviceWsServer] Cannot send to unknown device:" << deviceId;
        return;
    }
    QJsonObject msg;
    msg["type"] = type;
    msg["data"] = data;
    msg["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QByteArray payload = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    session->socket->sendTextMessage(QString::fromUtf8(payload));
}

void DeviceWebSocketServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QWebSocket *socket = m_server->nextPendingConnection();
        if (!socket) continue;

        auto *session = new DeviceSession{};
        session->socket = socket;
        m_socketSessions.insert(socket, session);

        connect(socket, &QWebSocket::disconnected,
                this, &DeviceWebSocketServer::onClientDisconnected);
        connect(socket, &QWebSocket::textMessageReceived,
                this, &DeviceWebSocketServer::onTextMessageReceived);

        qDebug() << "[DeviceWsServer] New WS connection from"
                 << socket->peerAddress().toString() << ":" << socket->peerPort();
    }
}

void DeviceWebSocketServer::onClientDisconnected()
{
    auto *socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {
        removeSession(socket);
    }
}

void DeviceWebSocketServer::onTextMessageReceived(const QString &message)
{
    auto *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    DeviceSession *session = m_socketSessions.value(socket);
    if (!session) return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[DeviceWsServer] Invalid JSON:" << err.errorString();
        return;
    }

    QJsonObject msg = doc.object();
    QString type = msg.value("type").toString();
    QString deviceId = msg.value("device_id").toString();
    QJsonObject payload = msg.value("payload").toObject();

    if (type == "register") {
        session->deviceId = deviceId;
        session->deviceName = payload.value("device").toObject().value("name").toString(deviceId);

        m_deviceSessions.insert(session->deviceId, session);
        qDebug() << "[DeviceWsServer] Device registered:" << session->deviceId;

        QJsonObject ack;
        ack["status"] = "ok";
        ack["device_id"] = session->deviceId;
        sendToDevice(session->deviceId, "register_ack", ack);

        emit deviceConnected(session->deviceId, session->deviceName);
    }

    if (!session->deviceId.isEmpty()) {
        emit jsonReceived(session->deviceId, type, payload);
    }
}

void DeviceWebSocketServer::removeSession(QWebSocket *socket)
{
    DeviceSession *session = m_socketSessions.take(socket);
    if (!session) return;

    QString deviceId = session->deviceId;
    if (!deviceId.isEmpty()) {
        m_deviceSessions.remove(deviceId);
        emit deviceDisconnected(deviceId);
        qDebug() << "[DeviceWsServer] Device disconnected:" << deviceId;
    }

    delete session;
}
