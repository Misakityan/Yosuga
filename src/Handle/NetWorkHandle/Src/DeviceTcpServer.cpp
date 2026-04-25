//
// Created by misaki on 2026/4/25.
//

#include "DeviceTcpServer.h"
#include <QDebug>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

DeviceTcpServer::DeviceTcpServer(quint16 port, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_port(port)
{
    connect(m_server, &QTcpServer::newConnection,
            this, &DeviceTcpServer::onNewConnection);
}

DeviceTcpServer::~DeviceTcpServer()
{
    stop();
}

bool DeviceTcpServer::start()
{
    if (m_server->isListening()) {
        qDebug() << "[DeviceTcpServer] Already listening on port" << m_port;
        return true;
    }
    if (!m_server->listen(QHostAddress::Any, m_port)) {
        QString err = QString("Failed to listen on TCP port %1: %2")
                          .arg(m_port).arg(m_server->errorString());
        qWarning() << "[DeviceTcpServer]" << err;
        emit serverError(err);
        return false;
    }
    qDebug() << "[DeviceTcpServer] Listening for embedded devices on TCP port" << m_port;
    return true;
}

void DeviceTcpServer::stop()
{
    for (auto it = m_socketSessions.begin(); it != m_socketSessions.end(); ++it) {
        DeviceSession *session = it.value();
        if (session->socket->state() == QAbstractSocket::ConnectedState) {
            session->socket->disconnectFromHost();
        }
        delete session;
    }
    m_deviceSessions.clear();
    m_socketSessions.clear();

    if (m_server->isListening()) {
        m_server->close();
    }
}

void DeviceTcpServer::sendToDevice(const QString &deviceId, const QString &type, const QJsonObject &data)
{
    DeviceSession *session = m_deviceSessions.value(deviceId);
    if (!session || !session->socket) {
        qWarning() << "[DeviceTcpServer] Cannot send to unknown device:" << deviceId;
        return;
    }
    QJsonObject msg;
    msg["type"] = type;
    msg["data"] = data;
    msg["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QByteArray payload = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    session->socket->write(payload);
    session->socket->write("\n");
    session->socket->flush();
}

void DeviceTcpServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        if (!socket) continue;

        auto *session = new DeviceSession{};
        session->socket = socket;
        m_socketSessions.insert(socket, session);

        connect(socket, &QTcpSocket::disconnected,
                this, &DeviceTcpServer::onClientDisconnected);
        connect(socket, &QTcpSocket::readyRead,
                this, &DeviceTcpServer::onReadyRead);

        qDebug() << "[DeviceTcpServer] New TCP connection from"
                 << socket->peerAddress().toString() << ":" << socket->peerPort();
    }
}

void DeviceTcpServer::onClientDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        removeSession(socket);
    }
}

void DeviceTcpServer::onReadyRead()
{
    auto *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    DeviceSession *session = m_socketSessions.value(socket);
    if (!session) return;

    session->buffer.append(socket->readAll());
    parseIncomingData(session);
}

void DeviceTcpServer::parseIncomingData(DeviceSession *session)
{
    // TCP: messages are newline-delimited JSON
    int newlinePos;
    while ((newlinePos = session->buffer.indexOf('\n')) >= 0) {
        QByteArray line = session->buffer.left(newlinePos).trimmed();
        session->buffer.remove(0, newlinePos + 1);

        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[DeviceTcpServer] Invalid JSON from device:" << err.errorString();
            continue;
        }

        QJsonObject msg = doc.object();
        QString type = msg.value("type").toString();
        QString deviceId = msg.value("device_id").toString();
        QJsonObject payload = msg.value("payload").toObject();

        // If this is a registration message, process it
        if (type == "register" || !session->deviceId.isEmpty()) {
            if (session->deviceId.isEmpty() && type == "register") {
                session->deviceId = deviceId;
                session->deviceName = payload.value("device").toObject().value("name").toString(deviceId);

                m_deviceSessions.insert(session->deviceId, session);
                qDebug() << "[DeviceTcpServer] Device registered:"
                         << session->deviceId << "(" << session->deviceName << ")";

                // Send ack
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
    }
}

void DeviceTcpServer::removeSession(QTcpSocket *socket)
{
    DeviceSession *session = m_socketSessions.take(socket);
    if (!session) return;

    QString deviceId = session->deviceId;
    if (!deviceId.isEmpty()) {
        m_deviceSessions.remove(deviceId);
        emit deviceDisconnected(deviceId);
        qDebug() << "[DeviceTcpServer] Device disconnected:" << deviceId;
    }

    delete session;
}
