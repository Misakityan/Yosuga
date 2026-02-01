//
// Created by Administrator on 2025/3/2.
//

#include "NetworkPage.h"
#include <QHBoxLayout>
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ElaMessageBar.h"
#include "websocketmanager.h"
#include "NetWorkDO.h"
#include <QFile>

NetWorkPage::NetWorkPage(QWidget* parent)
        : BasePage(parent)
{
    // 预览窗口标题
    setWindowTitle("NetworkPage");

    this->initUI();     // 初始化UI
    this->initWebSocketClient();    // 初始化websocket客户端(主要是相关的信号与槽)
}

NetWorkPage::~NetWorkPage()
{

}

void NetWorkPage::initUI() {
    // websocket UI
    websocketPushButton = new ElaPushButton("设定",this);
    websocketPushButton->setToolTip("设定服务端WebSocket地址");
    websocketLineEdit = new ElaLineEdit(this);
    websocketLineEdit->setPlaceholderText("请输入服务端WebSocket地址");
    websocketLineEdit->setFixedWidth(300);  // 设置websocketLineEdit框的宽度

    ElaScrollPageArea* websocketToggleSwitchArea = new ElaScrollPageArea(this);
    QHBoxLayout* websocketToggleSwitchLayout = new QHBoxLayout(websocketToggleSwitchArea);
    ElaText* websocketToggleSwitchText = new ElaText("服务端WebSocket地址:", this);
    websocketToggleSwitchText->setTextPixelSize(15);
    websocketToggleSwitchLayout->addWidget(websocketToggleSwitchText);
    websocketToggleSwitchLayout->addWidget(websocketLineEdit);
    websocketToggleSwitchLayout->addStretch();
    websocketToggleSwitchLayout->addWidget(websocketPushButton);
    websocketToggleSwitchLayout->addSpacing(10);
    // 连通测试按钮
    connectTestPushButton = new ElaPushButton("连通测试",this);
    connectTestPushButton->setToolTip("测试与服务器连通性(如果成功连通会自动连上服务器)");
    connectPushButton = new ElaPushButton("连接",this);
    disconnectPushButton = new ElaPushButton("断开",this);
    sendTestPushButton = new ElaPushButton("发送测试",this);
    ElaScrollPageArea* connectTestArea = new ElaScrollPageArea(this);   // 创建一个滚动页面
    QHBoxLayout* connectTestLayout = new QHBoxLayout(connectTestArea);
    connectTestLayout->addWidget(connectTestPushButton);        // 将连通测试按钮添加到布局中
    connectTestLayout->addStretch();                            // 添加一个空格
    connectTestLayout->addWidget(sendTestPushButton);           // 将发送测试按钮添加到布局中
    connectTestLayout->addWidget(disconnectPushButton);         // 将断开按钮添加到布局中
    connectTestLayout->addWidget(connectPushButton);            // 将连接按钮添加到布局中
    connectTestLayout->addSpacing(10);
    // 添加到布局
    QWidget* centralWidget = new QWidget(this);             // 中心部件
    centralWidget->setWindowTitle("连接设置");                  // 中心部件标题
    QVBoxLayout* centerLayout = new QVBoxLayout(centralWidget);   // 中心部件布局
    centerLayout->addWidget(websocketToggleSwitchArea);             // 将websocketToggleSwitchArea添加到布局中
    centerLayout->addWidget(connectTestArea);                       // 将connectTestArea添加到布局中

    centerLayout->addStretch();
    centerLayout->setContentsMargins(0, 0, 0, 0);   // 设置布局的边距
    addCentralWidget(centralWidget, true, true, 0); // 添加中心部件
}

void NetWorkPage::initWebSocketClient() {
    auto* client = WebSocketClient::getInstance();     // 获取单例实例(设置一个默认地址)
    auto* netDO = NetworkDO::getInstance();
    // 注入：将底层发送能力赋予 NetworkDO
    netDO->registerSender([client](const QString& type, const QJsonObject& data){
        client->sendJson(type, data);
    });
    // 监听：底层收到数据 -> NetworkDO 解析
    connect(client, &WebSocketClient::jsonReceived,
                 netDO, &NetworkDO::onDataReceived);
    // 连接成功的处理
    connect(client, &WebSocketClient::connected, this, [this]() {
        ElaMessageBar::success(ElaMessageBarType::TopRight, "WebSocket", "连接成功", 800.0, this);
    });
    // 连接失败的处理
    connect(client, &WebSocketClient::error, this, [this](const QString& errorMsg) {
        ElaMessageBar::error(ElaMessageBarType::TopLeft, "WebSocket错误", errorMsg, 1500.0, this);
    });
    // 断开连接的处理
    connect(client, &WebSocketClient::disconnected, this, [this]() {
        ElaMessageBar::information(ElaMessageBarType::BottomRight, "WebSocket", "连接已断开", 800.0, this);
    });
    // 接收数据处理
    connect(client, &WebSocketClient::jsonReceived, this, [](const QString &type, const QJsonObject &data) {
        qDebug() << "Received JSON data: " << type << " " << data;
    });

    connect(websocketPushButton, &ElaPushButton::clicked, this, [this, client]() {   // 设置服务端websocket地址
        const QUrl url(websocketLineEdit->text().trimmed());      // 从LineEdit中获取服务端websocket地址
        // 初始化客户端
        if (client->setConfiguration(url)) {
            ElaMessageBar::success(ElaMessageBarType::TopRight, "连接设置",
            QString("服务器地址已设置为: %1").arg(url.toString()), 800.0, this);
            return;
        }
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, "连接设置",
            QString("服务器地址存在问题"), 800.0, this);
    });

    connect(connectTestPushButton, &ElaPushButton::clicked, this, [this, client]() {
        if (client->isConnected()) {
            ElaMessageBar::success(ElaMessageBarType::TopRight, "连通测试",
                "当前已连通", 800.0, this);
            return;
        }
        client->connectToServer();      // 连接
        // 使用定时器延迟检查连接状态
        QTimer::singleShot(1000, this, [this, client]() {
            if (!client->isConnected()) {
                ElaMessageBar::warning(ElaMessageBarType::TopLeft, "连通测试",
                    "无法连接到服务器，请检查地址和服务器状态", 1500.0, this);
                return;
            }
            ElaMessageBar::success(ElaMessageBarType::TopRight, "连通测试",
                    "连通测试成功", 800.0, this);
        });
    });
    connect(connectPushButton, &ElaPushButton::clicked, this, [this, client]() {
        if (client->isConnected()) {
            ElaMessageBar::information(ElaMessageBarType::TopRight, "连接状态",
                "已连接，无需重复连接", 800.0, this);
            return;
        }
        const QString urlStr = websocketLineEdit->text().trimmed();
        if (urlStr.isEmpty()) {
            ElaMessageBar::warning(ElaMessageBarType::TopLeft, "连接",
                "请先设置服务器地址", 800.0, this);
            return;
        }
        // 确保使用正确的地址
        client->setConfiguration(QUrl(urlStr));
        client->connectToServer();
        // 连接结果会在 connected/error 信号中处理
        ElaMessageBar::information(ElaMessageBarType::TopRight, "连接",
            "正在连接服务器...", 800.0, this);
    });
    connect(disconnectPushButton, &ElaPushButton::clicked, this, [this, client]() {
        if (!client->isConnected()) {
            ElaMessageBar::information(ElaMessageBarType::BottomRight, "断开连接",
                "当前未连接", 800.0, this);
            return;
        }
        client->disconnectFromServer();
        ElaMessageBar::success(ElaMessageBarType::TopRight, "断开连接",
            "已断开连接", 800.0, this);
    });
    connect(sendTestPushButton, &ElaPushButton::clicked, this, [this, netDO, client]() {
        if (!client->isConnected()) {
            ElaMessageBar::information(ElaMessageBarType::BottomRight, "断开连接",
                "当前未连接", 800.0, this);
            return;
        }
        // 创建数据包
        QFile test_wav_file("Resources/TestFiles/test.wav");
        if (!test_wav_file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open test.wav";
            ElaMessageBar::warning(ElaMessageBarType::TopLeft, "发送测试",
            "无法打开测试音频文件", 1500.0, this);
            return;
        }
        QByteArray wavData = test_wav_file.readAll();
        QString base64Str = QString::fromLatin1(wavData.toBase64());
        AudioDataTransferObject packet;
        packet.setData("Owner", "client")
            .setData("isStream", true)
            .setData("sequence", 42)
            .setData("text", "Hello World")
            .setData("data", base64Str);   // 填入测试音频数据
        netDO->sendPacket(packet);
        ElaMessageBar::success(ElaMessageBarType::TopRight, "发送测试",
                "已成功发送数据包", 1000.0, this);
    });
}


