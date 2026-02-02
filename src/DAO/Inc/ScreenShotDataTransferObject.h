//
// Created by misaki on 2026/2/1.
//

/**
 * 数据传输对象 (DTO) 定义
 * ScreenShotDataTransferObject
 * 与Yosuga_server中的是对等DTO
 */

#pragma once
#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include "DataTransferObjectBase.h"
// 前向声明，减少依赖
class QJsonObject;
class ScreenShotDataTransferObject final : public DataTransferObjectBase{
public:
    // 构造函数（带默认值）
    explicit ScreenShotDataTransferObject(QString  owner = "client",
                                            bool isSuccess = true,
                                            QString  realtimeScreenShot = "",
                                            int width = 0, int height = 0,
                                            QString  describeInfo = "", QString LLMResponse =  ""
                                           );
    // 静态工厂方法
    static ScreenShotDataTransferObject fromJson(const QJsonObject& json);

    [[nodiscard]] QString type() const override { return "screenshot_data"; }

    // 序列化
    [[nodiscard]] QJsonObject toJson() const override;    // 通过多态即可统一调用方式

    // 链式调用设置
    ScreenShotDataTransferObject& setData(const QString& key, const QJsonValue& value) override;

    [[nodiscard]] QString owner() const { return m_owner; }
    [[nodiscard]] bool isSuccess() const { return m_isSuccess; }
    [[nodiscard]] QString realtimeScreenShot() const { return m_realtimeScreenShot; }
    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] QString describeInfo() const { return m_describeInfo; }
    [[nodiscard]] QString LLMResponse() const { return m_LLMResponse; }

private:
    QString m_owner;                        /// 数据的拥有者(server or client)
    bool    m_isSuccess;                    /// 截图是否成功
    QString m_realtimeScreenShot;           /// 客户端设备的实时截图数据(base64)
    int     m_width;                        /// 截图宽度 非必要字段
    int     m_height;                       /// 截图高度 非必要字段
    QString m_describeInfo;                 /// 设备的描述信息(告知模型以做出更加准确的判断) 非必要字段
    QString m_LLMResponse;                  /// LLM的响应结果(由服务端发送时携带)
};
