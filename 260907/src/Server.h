#pragma once
#include <map>
#include <vector>

class EventLoop;
class Acceptor;
class Socket;
class Connection;
class ThreadPool;

class Server
{
    private:
        EventLoop *mainReactor;
        Acceptor *acceptor;
        std::map<int,Connection*> connections;
        std::vector<EventLoop*> subReactors;
        ThreadPool *threadpool;
    public:
        Server(EventLoop*);
        ~Server();

        void handleReadEvent(int);
        void newConnection(Socket *sock);
        void deleteConnection(Socket *sock);
};