#include "Server.h"
#include "Socket.h"
#include "Connection.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include <functional>

Server::Server(EventLoop *_loop):loop(_loop),acceptor(nullptr)
{
    acceptor=new Acceptor(loop);
    std::function<void(Socket *)> cb=std::bind(&Server::newConnection,this,std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);
}


Server::~Server(){
    delete acceptor;
}


void Server::newConnection(Socket *sock){
    Connection *conn = new Connection(loop, sock);
    std::function<void(Socket*)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1);
    conn->setdeleteConnectionCallback(cb);
    connections[sock->getfd()] = conn;
}

void Server::deleteConnection(Socket * sock){
    Connection *conn = connections[sock->getfd()];
    connections.erase(sock->getfd());
    delete conn;
}