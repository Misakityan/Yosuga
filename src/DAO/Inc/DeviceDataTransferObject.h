//
// Created by Yosuga on 2026/4/25.
//

#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonDocument>
#include "DataTransferObjectBase.h"

class DeviceDataTransferObject final : public DataTransferObjectBase {
public:
    explicit DeviceDataTransferObject(
        QString action = "",
        QString deviceId = "",
        QJsonObject payload = {}
    );

    static DeviceDataTransferObject fromJson(const QJsonObject& json);

    // WebSocket 类型固定为 "device_data"
    [[nodiscard]] QString type() const override { return QStringLiteral("device_data"); }

    [[nodiscard]] QJsonObject toJson() const override;
    DeviceDataTransferObject& setData(const QString& key, const QJsonValue& value) override;

    [[nodiscard]] QString action() const { return m_action; }
    [[nodiscard]] QString deviceId() const { return m_deviceId; }
    [[nodiscard]] QJsonObject payload() const { return m_payload; }

private:
    QString m_action;
    QString m_deviceId;
    QJsonObject m_payload;
};
