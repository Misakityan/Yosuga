//
// Created by misaki on 2026/1/24.
//

#pragma once

/**
 *  客户端业务核心
 *  1. 处理来自服务端的数据，分发并执行
 *  2. 完成非阻塞的事件循环处理，构建业务状态机
 */
#include <QMutex>
#include <QObject>

class AppCore final : public QObject {
Q_OBJECT
Q_DISABLE_COPY(AppCore)     // 禁用拷贝

private:
    /**
     * 构造函数私有化
     * @param parent
     */
    explicit AppCore(QObject *parent = nullptr);        // 并不将本模块挂在对象树当中，因为本模块为单例类，内存自行管理

    static QScopedPointer<AppCore> m_instance;          // 单例类
    static QMutex m_mutex;
private slots:
    // 业务接收槽函数
    void onRecordingFinished_Byte(const QByteArray &wavData);
public:
    // 单例访问点
    static AppCore *getInstance();
    // 显式销毁
    static void destroy();

    ~AppCore() override;

public:
    // 单次对话
    void SingleExchange();
};