//
// Created by Administrator on 2025/3/2.
//
#pragma once

#include "BasePage.h"
#include "ElaPushButton.h"
#include "ElaLineEdit.h"

class ElaPushButton;
class ElaLineEdit;
class NetWorkPage final : public BasePage
{
Q_OBJECT
public:
    Q_INVOKABLE explicit NetWorkPage(QWidget* parent = nullptr);
    ~NetWorkPage() override;

private:
    void initUI();
    void initWebSocketClient();



private:
    // websocket 控件
    ElaPushButton* websocketPushButton = nullptr;
    ElaLineEdit* websocketLineEdit = nullptr;

    // 连接测试
    ElaPushButton* connectTestPushButton = nullptr;
    // 连接
    ElaPushButton* connectPushButton = nullptr;
    // 断开
    ElaPushButton* disconnectPushButton = nullptr;

    // 发送测试按钮
    ElaPushButton* sendTestPushButton = nullptr;
};
