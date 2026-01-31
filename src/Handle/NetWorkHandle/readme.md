当前项目中实现了tcp, socket, websocket三种通信方式
项目只用到了了**websocket**方式，其他两种是历史遗留(Yosuga[Qt5]所使用)
并且websocket经过了重构，交互数据也为自定义格式

这边顺便提供下WebSocket类的Mermaid图，帮助理解(将代码丢给AI生成出来的图，审阅了一下还是十分准确的)

## 1. 类图 

```mermaid
classDiagram
    class WebSocketManager {
        -QWebSocket* m_socket
        -QUrl m_url
        -QTimer* m_pingTimer
        -QTimer* m_reconnectTimer
        -int m_reconnectAttempts
        -bool m_isReconnectEnabled
        -QNetworkRequest m_request
        -bool m_isRequest
        +WebSocketManager(parent)
        ~WebSocketManager()
        +setRequestContent(requestToken) bool
        +setSocketUrl(url) bool
        +connectToServer() bool
        +disconnectFromServer()
        +sendText(message)
        +sendJson(type, data)
        +sendBinary(data)
        +setReconnectEnabled(enabled)
        +setRequestEnabled(enabled)
        +isConnected() bool
        -onConnected()
        -onDisconnected()
        -onTextMessageReceived(message)
        -onError(socketError)
        -onSslErrors(errors)
        -onPong(elapsedTime, payload)
        -sendPing()
        -tryReconnect()
        -- signals --
        +connected()
        +disconnected()
        +textReceived(message)
        +jsonReceived(type, data)
        +binaryReceived(data)
        +error(errorMsg)
        +log(msg)
        +reconnecting(attempt)
    }

    class WebSocketClient {
        -static QMutex m_mutex
        -static QScopedPointer~WebSocketClient~ m_instance
        -QThread* m_workerThread
        -WebSocketManager* m_webSocketManager
        -QUrl m_url
        -QString m_authToken
        -bool m_hasAuthToken
        +getInstance() WebSocketClient*
        +destroy()
        +setConfiguration(url, authToken) bool
        +connectToServer()
        +disconnectFromServer()
        +reconnect()
        +sendText(message)
        +sendJson(type, data)
        +sendBinary(data)
        +isConnected() bool
        +hasConfiguration() bool
        +setAutoReconnect(enabled)
        +setPingInterval(milliseconds)
        +currentUrl() QUrl
        +currentToken() QString
        +manager() WebSocketManager*
        -- signals --
        +connected()
        +disconnected()
        +textReceived(message)
        +jsonReceived(type, data)
        +binaryReceived(data)
        +error(errorMsg)
        +log(msg)
        +reconnecting(attempt)
        +configurationChanged(oldUrl, newUrl)
        -internalSetUrl(url)
        -internalSetAuthToken(token)
        -internalConnect()
        -internalDisconnect()
        -internalSendText(message)
        -internalSendJson(type, data)
        -internalSendBinary(data)
        -internalSetAutoReconnect(enabled)
        -internalSetRequestEnabled(enabled)
    }

    class QWebSocket {
        +open(url)
        +close()
        +sendTextMessage(message)
        +sendBinaryMessage(data)
        +ping()
        +state() QAbstractSocket::SocketState
    }

    class QThread {
        +start()
        +quit()
        +wait()
        +terminate()
        +isRunning() bool
    }

    WebSocketClient "1" --> "1" WebSocketManager : 管理
    WebSocketClient "1" --> "1" QThread : 工作线程
    WebSocketManager "1" --> "1" QWebSocket : 封装
    WebSocketManager "1" --> "2" QTimer : 心跳/重连
```

## 2. 连接时序图 

```mermaid
sequenceDiagram
    participant MainThread as 主线程
    participant Client as WebSocketClient
    participant WorkerThread as 工作线程
    participant Manager as WebSocketManager
    participant Socket as QWebSocket
    participant Server as WebSocket服务器

    MainThread->>Client: getInstance()
    Client-->>MainThread: WebSocketClient实例
    
    MainThread->>Client: setConfiguration(url, token)
    Client->>WorkerThread: 创建工作线程
    Client->>Manager: 创建WebSocketManager
    Manager->>Socket: 创建QWebSocket
    Client->>Manager: 信号连接配置
    Client->>Manager: 设置URL和Token
    
    MainThread->>Client: connectToServer()
    Client->>Manager: emit internalConnect()
    Manager->>Socket: open(url/request)
    Socket->>Server: WebSocket握手
    Server-->>Socket: 连接成功
    Socket->>Manager: connected()
    Manager->>Manager: 启动心跳定时器
    Manager->>Client: emit connected()
    Client->>MainThread: emit connected()
```

## 3. 状态图 

```mermaid
stateDiagram-v2
    [*] --> 未配置
    
    未配置 --> 已配置 : setConfiguration()
    已配置 --> 连接中 : connectToServer()
    
    连接中 --> 已连接 : 连接成功
    连接中 --> 重连中 : 连接失败
    连接中 --> [*] : disconnectFromServer()
    
    已连接 --> 已连接 : 发送/接收数据
    已连接 --> 已断开 : 连接断开
    已连接 --> [*] : disconnectFromServer()
    
    已断开 --> 重连中 : 自动重连开启
    已断开 --> [*] : 自动重连关闭
    
    重连中 --> 已连接 : 重连成功
    重连中 --> 重连中 : 重连失败(继续重试)
    重连中 --> [*] : disconnectFromServer()
    
    state 重连中 {
        [*] --> 等待重试
        等待重试 --> 尝试连接 : 定时器触发
        尝试连接 --> 等待重试 : 连接失败
        尝试连接 --> [*] : 连接成功
    }
```

## 4. 消息发送时序图 

```mermaid
sequenceDiagram
    participant App as 应用程序
    participant Client as WebSocketClient
    participant Manager as WebSocketManager
    participant Socket as QWebSocket
    participant Server as WebSocket服务器

    App->>Client: sendJson("message", data)
    alt 已连接
        Client->>Manager: emit internalSendJson("message", data)
        Manager->>Socket: sendTextMessage(json)
        Socket->>Server: 发送JSON数据
        Server-->>Socket: 响应(可选)
        Socket->>Manager: textMessageReceived()
        Manager->>Manager: 解析JSON
        Manager->>Client: emit jsonReceived()
        Client->>App: emit jsonReceived()
    else 未连接
        Client->>App: emit error("未连接")
    end
```

## 5. 线程关系图 

```mermaid
flowchart TD
    subgraph 主线程 [UI/主线程]
        direction LR
        App[应用程序]
        Client[WebSocketClient<br/>单例]
    end
    
    subgraph 工作线程 [WebSocket工作线程]
        direction LR
        Manager[WebSocketManager]
        Socket[QWebSocket]
    end
    
    subgraph 外部系统
        Server[WebSocket服务器]
    end
    
    App -- 调用 --> Client
    Client -- 跨线程信号 --> Manager
    Manager -- Qt信号槽 --> Socket
    Socket -- TCP/WebSocket --> Server
    
    Client -- 返回事件信号 --> App
    Socket -- 接收数据 --> Manager
    Manager -- 跨线程信号 --> Client
```