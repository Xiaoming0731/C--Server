// #pragma once
// #include <functional>
// #include "Buffer.h"

// class EventLoop;
// class Channel;
// class Socket;
// class Buffer;

// class Connection
// {
//     public:
//         enum State{
//         Invalid=1,
//         Handshaking,
//         Connected,
//         Closed,
//         Failed,
//         };
//         Connection(EventLoop *loop,Socket *sock);
//         ~Connection();

//         void Read();
//         void Write();
//         void clese();

//         State GetState();
//         void Close();
//         void SetSendBuffer(const char *str);
//         Buffer *GetReadBuffer();
//         const char *ReadBuffer();
//         Buffer *GetSendBuffer();
//         const char *SendBuffer();
//         void GetlineSendBuffer();
//         Socket *GetSocket();

//         void setdeleteConnectionCallback(std::function<void(Socket*)>);
//         void SetOnConnectCallback(std::function<void(Connection *)> const &callback);
        

//         void OnConnect(std::function<void()> fn);

//     private:
//         EventLoop *loop;
//         Channel *channel;
//         Socket *sock;

//         Buffer *inBuffer;   // 输入缓冲区：暂存读到的数据
//         Buffer *outBuffer;  // 输出缓冲区：暂存待发送的数据

//         State state_{State::Invalid};

//         void handleRead();    // EPOLLIN 触发：读数据 + 业务处理
//         void handleWrite();   // EPOLLOUT 触发：继续发送剩余数据
//         void sendOutBuffer(); // 核心：尝试把 outBuffer 写入 socket，管理 EPOLLOUT 开关
    
//         std::function<void(Socket *)> delete_connectioin_callback_;

//         std::function<void(Connection *)> on_connect_callback_;

//         void ReadNonBlocking();
//         void WriteNonBlocking();
// };


#pragma once
#include <functional>
#include "Buffer.h"

class EventLoop;
class Channel;
class Socket;

class Connection
{
public:
    enum State {
        Invalid = 1,
        Handshaking,
        Connected,
        Closed,
        Failed,
    };

    Connection(EventLoop *loop, Socket *sock);
    ~Connection();

    void Read();
    void Write();
    void Close();

    State GetState();
    void SetSendBuffer(const char *str);
    void GetlineSendBuffer();

    Buffer *GetReadBuffer();
    const char *ReadBuffer();
    Buffer *GetSendBuffer();
    const char *SendBuffer();
    Socket *GetSocket();

    void SetDeleteConnectionCallback(std::function<void(Socket *)> const &callback);
    void SetOnConnectCallback(std::function<void(Connection *)> const &callback);

private:
    EventLoop *loop_;
    Channel *channel_;
    Socket *sock_;

    Buffer *read_buffer_;
    Buffer *send_buffer_;
    State state_;

    std::function<void(Socket *)> delete_connection_callback_;
    std::function<void(Connection *)> on_connect_callback_;

    void ReadNonBlocking();
    void WriteNonBlocking();
};
