//
// Created by misaki on 2026/2/1.
//
#include "ScreenShotDataTransferObject.h"
#include <QJsonValue>
#include <utility>
#include <QDebug>

// 构造函数实现（初始化列表）
ScreenShotDataTransferObject::ScreenShotDataTransferObject(QString owner,
                                                         bool isSuccess,
                                                         QString realtimeScreenShot,
                                                         int width, int height,
                                                         QString describeInfo, QString LLMResponse)
    : m_owner(std::move(owner))
    , m_isSuccess(isSuccess)
    , m_realtimeScreenShot(std::move(realtimeScreenShot))
    , m_width(width)
    , m_height(height)
    , m_describeInfo(std::move(describeInfo))
    , m_LLMResponse(std::move(LLMResponse))
{}

// 静态工厂方法：从 JSON 反序列化
ScreenShotDataTransferObject ScreenShotDataTransferObject::fromJson(const QJsonObject& json) {
    // 逐个字段读取，不存在则用默认值
    const QString owner = json.value("Owner").toString("client");
    const bool isSuccess = json.value("isSuccess").toBool(false);
    const QString realtimeScreenShot = json.value("RealTimeScreenShot").toString();
    const int width = json.value("Width").toInt(0);
    const int height = json.value("Height").toInt(0);
    const QString describeInfo = json.value("DescribeInfo").toString();
    const QString LLMResponse = json.value("LLMResponse").toString();

    // 调用构造函数创建对象
    return ScreenShotDataTransferObject(owner, isSuccess, realtimeScreenShot,
                                      width, height, describeInfo);
}

// 序列化为 JSON
QJsonObject ScreenShotDataTransferObject::toJson() const {
    QJsonObject json;
    json["Owner"] = m_owner;
    json["isSuccess"] = m_isSuccess;
    json["RealTimeScreenShot"] = m_realtimeScreenShot;
    json["Width"] = m_width;
    json["Height"] = m_height;
    json["DescribeInfo"] = m_describeInfo;
    json["LLMResponse"] = m_LLMResponse;
    return json;
}

// 链式设置
ScreenShotDataTransferObject& ScreenShotDataTransferObject::setData(const QString& key,
                                                                    const QJsonValue& value) {
    if (key == "Owner") {
        m_owner = value.toString();
    } else if (key == "isSuccess") {
        m_isSuccess = value.toBool();
    } else if (key == "RealTimeScreenShot") {
        m_realtimeScreenShot = value.toString();
    } else if (key == "Width") {
        m_width = value.toInt();
    } else if (key == "Height") {
        m_height = value.toInt();
    } else if (key == "DescribeInfo") {
        m_describeInfo = value.toString();
    } else if (key == "LLMResponse") {
        m_LLMResponse = value.toString();
    } else {
        qWarning() << "Unknown key:" << key << "for ScreenShotDataTransferObject";
    }

    return *this;  // 返回自身引用，支持链式调用
}