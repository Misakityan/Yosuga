//
// Created by misaki on 2026/2/1.
//
#include "ScreenShotReqDataHandle.h"
#include "NetWorkDO.h"
#include <QDebug>
#include <QPixmap>
#include "ScreenShotDataTransferObject.h"
#include "ScreenHelperUtil.hpp"
// 初始化静态成员
QScopedPointer<ScreenShotReqDataHandle> ScreenShotReqDataHandle::m_instance;
QMutex ScreenShotReqDataHandle::m_mutex;

// 单例实现 (QScopedPointer + Mutex)
ScreenShotReqDataHandle* ScreenShotReqDataHandle::getInstance()
{
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            // 使用 reset 创建实例，因为构造函数是私有的
            m_instance.reset(new ScreenShotReqDataHandle());
        }
    }
    return m_instance.data();
}

void ScreenShotReqDataHandle::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset(); // 这会触发析构函数
    }
}


ScreenShotReqDataHandle::ScreenShotReqDataHandle(QObject *parent) : QObject(parent)
{
    connect(NetworkDO::getInstance(), &NetworkDO::screenShotPacketReceived,
            this, &ScreenShotReqDataHandle::onScreenShotPacketReceived);
    // 初始化时候就构造好关于当前运行平台的信息
    ScreenHelper::SystemInfo sysInfo = ScreenHelper::getSystemInfo();
    const QString sysText = QString("System: %1  OS Version: %2  Display Server: %3")
                          .arg(sysInfo.osType, sysInfo.osVersion, sysInfo.displayServer);
    this->m_systemInfo = sysText;
}

ScreenShotReqDataHandle::~ScreenShotReqDataHandle()
{
    qDebug() << "ScreenShotDataHandle destroyed";
}

void ScreenShotReqDataHandle::onScreenShotPacketReceived(const ScreenShotDataTransferObject &packet) const {
    qDebug() << "ScreenShot packet request from:" << packet.owner();
    // 截图当前画面并构造对等DTO发送
    const ScreenHelper::ScreenshotResult result = ScreenHelper::captureFocusedScreen();   // 获取当前屏幕截图
    if (!result.success) {  // 如果截图失败
        // TODO: 考虑失败时候构造一个错误DTO给服务端
        return;
    }
    ScreenShotDataTransferObject reback;    // 构造返回的DTO
    reback.setData("isSuccess", true).setData("RealTimeScreenShot", result.base64Data)
        .setData("Width", result.width).setData("Height", result.height)
        .setData("DescribeInfo", this->m_systemInfo);
    // 发送DTO
    NetworkDO::getInstance()->sendPacket(reback);
}
