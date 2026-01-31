//
// Created by misaki on 2026/1/30.
//

/**
 * 本模块通过解析AutoAgentDataObject的内容并调用 AutoGUI 模块
 * 来完成自动化GUI操作
 * 对于GUI自动化执行器而言，运行时只需要有一个实例即可，因此采用单例模式，并在AppCore当中进行创建
 */

#pragma once
#include <QObject>
#include <QMutex>
#include "AutoAgentDataObject.h"

class AutoAgentHandle final : public QObject
{
Q_OBJECT
Q_DISABLE_COPY(AutoAgentHandle)     // 禁用拷贝
private:
    /**
     * 构造函数私有化
     * @param parent
     */
    explicit AutoAgentHandle(QObject *parent = nullptr);        // 并不将本模块挂在对象树当中，因为本模块为单例类，内存自行管理

    static QScopedPointer<AutoAgentHandle> m_instance;       // 单例类
    static QMutex m_mutex;
private slots:
    // 业务接收槽函数，当获取到自动化agent数据包时，进行解析并调用 AutoGUI 模块
    void onAutoAgentPacketReceived(const AutoAgentDataObject& packet);
public:
    // 单例访问点
    static AutoAgentHandle *getInstance();
    // 显式销毁
    static void destroy();

    ~AutoAgentHandle() override;
};

