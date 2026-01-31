//
// Created by misaki on 2026/1/30.
//

/**
 * 自动代理数据对象
 * 非对等传输对象，只被用于将服务端返回的auto_agent json转换为对象
 */

#pragma once
#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include "DataTransferObjectBase.h"
class QJsonObject;
class AutoAgentDataObject final : public DataTransferObjectBase {
public:
    // 构造函数（带默认值）
    explicit AutoAgentDataObject(const QString& Action,
                                        int X1,
                                        int Y1,
                                        int X2,
                                        int Y2,
                                        const QString& Key,
                                        const QString& Content,
                                        const QString& Direction);
    // 静态工厂方法
    static AutoAgentDataObject fromJson(const QJsonObject& json);

    [[nodiscard]] QString type() const override { return "auto_agent"; }

    [[nodiscard]] QJsonObject toJson() const override;    // 通过多态即可统一调用方式

    // 链式调用设置
    AutoAgentDataObject& setData(const QString& key, const QJsonValue& value) override;

    [[nodiscard]] QString getAction() const { return m_action; }
    [[nodiscard]] int getX1() const { return m_x1; }
    [[nodiscard]] int getY1() const { return m_y1; }
    [[nodiscard]] int getX2() const { return m_x2; }
    [[nodiscard]] int getY2() const { return m_y2; }
    [[nodiscard]] QString getKey() const { return m_key; }
    [[nodiscard]] QString getContent() const { return m_content; }
    [[nodiscard]] QString getDirection() const { return m_direction; }

private:
    QString m_action;               /// 自动化动作名称
    int m_x1;                       /// 鼠标起始位置x1
    int m_y1;                       /// 鼠标起始位置y1
    int m_x2;                       /// 鼠标结束位置x2
    int m_y2;                       /// 鼠标结束位置y2
    QString m_key;                  /// 快捷键
    QString m_content;              /// 输入文本内容
    QString m_direction;            /// 滚动方向
};