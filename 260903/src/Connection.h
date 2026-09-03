#pragma once
#include <functional>

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
    public:
        Connection(EventLoop *loop,Socket *sock);
        ~Connection();

        void echo(int sockfd);
        void setdeleteConnectionCallback(std::function<void(Socket*)>);
};