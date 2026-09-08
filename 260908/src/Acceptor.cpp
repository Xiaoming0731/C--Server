// #include "Acceptor.h"
// #include "Socket.h"
// #include "InetAddress.h"
// #include "Channel.h"
// #include "EventLoop.h"
// #include <stdio.h>

// Acceptor::Acceptor(EventLoop *_loop) : loop(_loop), sock(nullptr), acceptChannel(nullptr){
//     sock = new Socket();
//     InetAddress *addr = new InetAddress("127.0.0.1", 1234);
//     sock->bind(addr);
//     sock->listen(); 
//     // sock->setnonblocking();
//     acceptChannel = new Channel(loop, sock->getfd());
//     std::function<void()> cb = std::bind(&Acceptor::acceptConnection, this);
//     acceptChannel->setReadCallback(cb);
//     // 删掉：acceptChannel->enableReading();
//     acceptChannel->setEvents(EPOLLIN);  // 只开EPOLLIN，默认LT水平触发
//     loop->updateChannel(acceptChannel);

//     delete addr;
// }

// Acceptor::~Acceptor(){
//     delete sock;
//     delete acceptChannel;
// }

// void Acceptor::acceptConnection(){
//     InetAddress *clnt_addr = new InetAddress();      
//     Socket *clnt_sock = new Socket(sock->accept(clnt_addr));      
//     printf("new client fd %d! IP: %s Port: %d\n", clnt_sock->getfd(), inet_ntoa(clnt_addr->getaddr().sin_addr), ntohs(clnt_addr->getaddr().sin_port));
//     clnt_sock->setnonblocking();
//     newConnectionCallback(clnt_sock);
//     delete clnt_addr;
// }

// void Acceptor::setNewConnectionCallback(std::function<void(Socket*)> _cb){
//     newConnectionCallback = _cb;
// }



#include "Acceptor.h"
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include <stdio.h>

Acceptor::Acceptor(EventLoop *loop) : loop_(loop), sock_(nullptr), acceptChannel_(nullptr){
    sock_ = new Socket();
    InetAddress *addr = new InetAddress("127.0.0.1", 1234);
    sock_->bind(addr);
    sock_->listen(); 

    acceptChannel_ = new Channel(loop_, sock_->getfd());
    std::function<void()> cb = std::bind(&Acceptor::acceptConnection, this);
    acceptChannel_->setReadCallback(cb);
    acceptChannel_->setEvents(EPOLLIN);
    loop_->updateChannel(acceptChannel_);
    delete addr;
}

Acceptor::~Acceptor(){
    delete sock_;
    delete acceptChannel_;
}

void Acceptor::acceptConnection(){
    InetAddress *clnt_addr = new InetAddress();      
    Socket *clnt_sock = new Socket(sock_->accept(clnt_addr));      
    printf("new client fd %d! IP: %s Port: %d\n", clnt_sock->getfd(), inet_ntoa(clnt_addr->getaddr().sin_addr), ntohs(clnt_addr->getaddr().sin_port));
    clnt_sock->setnonblocking();
    newConnectionCallback_(clnt_sock);
    delete clnt_addr;
}

void Acceptor::setNewConnectionCallback(std::function<void(Socket*)> cb){
    newConnectionCallback_ = cb;
}
