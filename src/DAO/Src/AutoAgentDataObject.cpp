//
// Created by misaki on 2026/1/30.
//
#include "AutoAgentDataObject.h"

AutoAgentDataObject::AutoAgentDataObject(const QString& Action,
                                         const int X1,
                                         const int Y1,
                                         const int X2,
                                         const int Y2,
                                         const QString& Key,
                                         const QString& Content,
                                         const QString& Direction)
    : m_action(Action),
      m_x1(X1),
      m_y1(Y1),
      m_x2(X2),
      m_y2(Y2),
      m_key(Key),
      m_content(Content),
      m_direction(Direction) {}

AutoAgentDataObject AutoAgentDataObject::fromJson(const QJsonObject& json) {
    // 从JSON对象中提取数据，如果不存在则使用默认值
    const QString action = json.value("Action").toString("");
    const int x1 = json.value("x1").toInt(-1);
    const int y1 = json.value("y1").toInt(-1);
    const int x2 = json.value("x2").toInt(-1);
    const int y2 = json.value("y2").toInt(-1);
    const QString key = json.value("key").toString("");
    const QString content = json.value("content").toString("");
    const QString direction = json.value("direction").toString("");

    return AutoAgentDataObject(action, x1, y1, x2, y2, key, content, direction);
}

QJsonObject AutoAgentDataObject::toJson() const {
    QJsonObject json;
    json["Action"] = m_action;
    json["x1"] = m_x1;
    json["y1"] = m_y1;
    json["x2"] = m_x2;
    json["y2"] = m_y2;
    json["key"] = m_key;
    json["content"] = m_content;
    json["direction"] = m_direction;
    return json;
}

AutoAgentDataObject& AutoAgentDataObject::setData(const QString& key, const QJsonValue& value) {
    // 根据键名设置对应的成员变量
    if (key == "Action" && value.isString()) {
        m_action = value.toString();
    } else if (key == "x1" && (value.isDouble() || value.isString())) {
        m_x1 = value.toInt();
    } else if (key == "y1" && (value.isDouble() || value.isString())) {
        m_y1 = value.toInt();
    } else if (key == "x2" && (value.isDouble() || value.isString())) {
        m_x2 = value.toInt();
    } else if (key == "y2" && (value.isDouble() || value.isString())) {
        m_y2 = value.toInt();
    } else if (key == "key" && value.isString()) {
        m_key = value.toString();
    } else if (key == "content" && value.isString()) {
        m_content = value.toString();
    } else if (key == "direction" && value.isString()) {
        m_direction = value.toString();
    } else {
        qWarning() << "Unknown key or invalid value type:" << key << value;
    }
    return *this;
}