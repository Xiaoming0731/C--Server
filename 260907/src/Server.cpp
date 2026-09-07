#include "Server.h"
#include "Socket.h"
#include "Connection.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "ThreadPool.h"
#include <functional>
#include <unistd.h>
#include <vector>
#include <thread>


Server::Server(EventLoop *_loop):mainReactor(_loop),acceptor(nullptr)
{
    acceptor=new Acceptor(mainReactor);
    std::function<void(Socket *)> cb=std::bind(&Server::newConnection,this,std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);

    int size=std::thread::hardware_concurrency();
    threadpool=new ThreadPool(size);
    for(int i=0;i<size;i++)
    {
        subReactors.push_back(new EventLoop());
    }

    for(int i=0;i<size;i++)
    {
        std::function<void()> sub_loop=std::bind(&EventLoop::loop,subReactors[i]);
        threadpool->add(sub_loop);
    }
}


Server::~Server(){
    delete acceptor;
    delete threadpool;
}


void Server::newConnection(Socket *sock){
    if(sock->getfd()!=-1)
    {
        int random=sock->getfd()%subReactors.size();
        Connection *conn=new Connection(subReactors[random],sock);
        std::function<void(Socket*)> cb=std::bind(&Server::deleteConnection,this,std::placeholders::_1);
        conn->setdeleteConnectionCallback(cb);
        connections[sock->getfd()]=conn;
    }
}

void Server::deleteConnection(Socket * sock){
    if(sock->getfd()!=-1)
    {
        auto it=connections.find(sock->getfd());
        if(it!=connections.end())
        {
            Connection *conn=connections[sock->getfd()];
            connections.erase(sock->getfd());
            delete conn;
        }
    }
}