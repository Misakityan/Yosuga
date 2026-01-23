//
// Created by misaki on 2026/1/13.
//
#pragma once

#include <QObject>

class DataTransferObjectBase
{
public:
    virtual ~DataTransferObjectBase() = default;

    // 获取类型，用于区分不同的DTO子类对象
    [[nodiscard]] virtual QString type() const = 0;

    // 序列化
    [[nodiscard]] virtual QJsonObject toJson() const = 0;

    // 链式调用设置
    virtual DataTransferObjectBase& setData(const QString& key, const QJsonValue& value) = 0;
};
