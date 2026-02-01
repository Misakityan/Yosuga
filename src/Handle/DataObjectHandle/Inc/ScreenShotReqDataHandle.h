//
// Created by misaki on 2026/2/1.
//
#pragma once
#include <QObject>
#include <QMutex>
#include "ScreenShotDataTransferObject.h"
class ScreenShotReqDataHandle final : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(ScreenShotReqDataHandle)     // 禁用拷贝
    private:
    /**
     * 构造函数私有化
     * @param parent
     */
    explicit ScreenShotReqDataHandle(QObject *parent = nullptr);        // 并不将本模块挂在对象树当中，因为本模块为单例类，内存自行管理

    static QScopedPointer<ScreenShotReqDataHandle> m_instance;       // 单例类
    static QMutex m_mutex;

private slots:
    // 业务接收槽函数，当获取到截图数据包时，进行解析并处理
    void onScreenShotPacketReceived(const ScreenShotDataTransferObject& packet) const;

    signals:
    // 发送截图处理完成的信号，供界面显示使用
    void screenShotProcessed(const QPixmap& screenshot, const QString& description);
public:
    // 单例访问点
    static ScreenShotReqDataHandle *getInstance();
    // 显式销毁
    static void destroy();

    ~ScreenShotReqDataHandle() override;
private:
    QString m_systemInfo;
};