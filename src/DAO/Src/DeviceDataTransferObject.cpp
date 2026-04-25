//
// Created by Yosuga on 2026/4/25.
//

#include "DeviceDataTransferObject.h"

DeviceDataTransferObject::DeviceDataTransferObject(
    QString action,
    QString deviceId,
    QJsonObject payload
) : m_action(std::move(action))
  , m_deviceId(std::move(deviceId))
  , m_payload(std::move(payload))
{
}

DeviceDataTransferObject DeviceDataTransferObject::fromJson(const QJsonObject& json)
{
    DeviceDataTransferObject obj;
    obj.m_action = json.value("action").toString("device_command");
    obj.m_deviceId = json.value("device_id").toString("");
    QJsonValue payloadVal = json.value("payload");
    if (payloadVal.isString()) {
        obj.m_payload["rpc_call"] = payloadVal.toString();
    } else if (payloadVal.isObject()) {
        obj.m_payload = payloadVal.toObject();
    }
    return obj;
}

QJsonObject DeviceDataTransferObject::toJson() const
{
    QJsonObject json;
    json["action"] = m_action;
    if (!m_deviceId.isEmpty()) {
        json["device_id"] = m_deviceId;
    }
    json["payload"] = m_payload;
    return json;
}

DeviceDataTransferObject& DeviceDataTransferObject::setData(const QString& key, const QJsonValue& value)
{
    if (key == "action") m_action = value.toString();
    else if (key == "device_id") m_deviceId = value.toString();
    else if (key == "payload") m_payload = value.toObject();
    return *this;
}
