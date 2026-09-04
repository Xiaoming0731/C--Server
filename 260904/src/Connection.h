#pragma once
#include <functional>
#include "Buffer.h"

class EventLoop;
class Channel;
class Socket;

class Connection
{
    private:
        EventLoop *loop;
        Channel *channel;
        Socket *sock;
        std::function<void(Socket*)> deleteConnectionCallback;

        Buffer inBuffer;   // 输入缓冲区：暂存读到的数据
        Buffer outBuffer;  // 输出缓冲区：暂存待发送的数据

        void handleRead();    // EPOLLIN 触发：读数据 + 业务处理
        void handleWrite();   // EPOLLOUT 触发：继续发送剩余数据
        void sendOutBuffer(); // 核心：尝试把 outBuffer 写入 socket，管理 EPOLLOUT 开关
    public:
        Connection(EventLoop *loop,Socket *sock);
        ~Connection();

        void setdeleteConnectionCallback(std::function<void(Socket*)>);
};