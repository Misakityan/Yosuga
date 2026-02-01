//
// Created by misaki on 2026/1/24.
//

#include "AppCore.h"
#include <QDebug>

#include "AudioDataHandle.h"
#include "AutoAgentHandle.h"
#include "ScreenShotReqDataHandle.h"

// 初始化静态成员
QScopedPointer<AppCore> AppCore::m_instance;
QMutex AppCore::m_mutex;

// 单例实现 (QScopedPointer + Mutex)
AppCore* AppCore::getInstance()
{
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            // 使用 reset 创建实例，因为构造函数是私有的
            m_instance.reset(new AppCore());
        }
    }
    return m_instance.data();
}

void AppCore::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset(); // 这会触发析构函数
    }
}

AppCore::AppCore(QObject *parent) : QObject(parent)
{
    // 初始化业务解析单例
    AudioDataHandle::getInstance();
    AutoAgentHandle::getInstance();
    ScreenShotReqDataHandle::getInstance();
}

AppCore::~AppCore()
{
    // 析构业务解析单例
    ScreenShotReqDataHandle::destroy();
    AutoAgentHandle::destroy();     // 显式销毁
    AudioDataHandle::destroy();


    qDebug() << "AppCore destroyed";
}
