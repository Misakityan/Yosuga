//
// Created by misaki on 2026/1/30.
//

#include "AutoAgentHandle.h"
#include "NetWorkDO.h"
#include <SimpleAutoGUI.h>          // 引入 AutoGUI 头文件
// 初始化静态成员
QScopedPointer<AutoAgentHandle> AutoAgentHandle::m_instance;
QMutex AutoAgentHandle::m_mutex;

// 单例实现 (QScopedPointer + Mutex)
AutoAgentHandle* AutoAgentHandle::getInstance()
{
    if (m_instance.isNull()) {
        QMutexLocker locker(&m_mutex);
        if (m_instance.isNull()) {
            // 使用 reset 创建实例，因为构造函数是私有的
            m_instance.reset(new AutoAgentHandle());
        }
    }
    return m_instance.data();
}

void AutoAgentHandle::destroy()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance.isNull()) {
        m_instance.reset(); // 这会触发析构函数
    }
}


AutoAgentHandle::AutoAgentHandle(QObject *parent) : QObject(parent)
{
    connect(NetworkDO::getInstance(), &NetworkDO::autoAgentPacketReceived, this, &AutoAgentHandle::onAutoAgentPacketReceived);
}

AutoAgentHandle::~AutoAgentHandle()
{
    qDebug() << "AutoAgentHandle destroyed";
}

void AutoAgentHandle::onAutoAgentPacketReceived(const AutoAgentDataObject &packet) {
    if (packet.getAction() == "click") {    // 单击
        qDebug() << "Click: " << packet.getX1() << ", " << packet.getY1();
        AutoGUI::moveTo(packet.getX1(), packet.getY1(), 0.6);
        AutoGUI::click(packet.getX1(), packet.getY1());
    }
    if (packet.getAction() == "left_double") {  // 双击
        qDebug() << "Double click: " << packet.getX1() << ", " << packet.getY1();
        AutoGUI::moveTo(packet.getX1(), packet.getY1(), 0.6);
        AutoGUI::leftDouble(packet.getX1(), packet.getY1());
    }
    if (packet.getAction() == "right_single") { // 右键单击
        qDebug() << "Right click: " << packet.getX1() << ", " << packet.getY1();
        AutoGUI::moveTo(packet.getX1(), packet.getY1(), 0.6);
        AutoGUI::rightSingle(packet.getX1(), packet.getY1());
    }
    if (packet.getAction() == "drag") {         // 拖拽
        qDebug() << "Drag: " << packet.getX1() << ", " << packet.getY1() << " to " << packet.getX2() << ", " << packet.getY2();
        AutoGUI::drag(packet.getX1(), packet.getY1(), packet.getX2(), packet.getY2(), 0.8);
    }
    // TODO: 快捷键，输入文本，滚动待实现
}
