// #include "Server.h"
// #include "Socket.h"
// #include "Connection.h"
// #include "Acceptor.h"
// #include "EventLoop.h"
// #include "ThreadPool.h"
// #include <functional>
// #include <unistd.h>
// #include <vector>
// #include <thread>


// Server::Server(EventLoop *_loop):mainReactor(_loop),acceptor(nullptr)
// {
//     acceptor=new Acceptor(mainReactor);
//     std::function<void(Socket *)> cb=std::bind(&Server::newConnection,this,std::placeholders::_1);
//     acceptor->setNewConnectionCallback(cb);

//     int size=std::thread::hardware_concurrency();
//     threadpool=new ThreadPool(size);
//     for(int i=0;i<size;i++)
//     {
//         subReactors.push_back(new EventLoop());
//     }

//     for(int i=0;i<size;i++)
//     {
//         std::function<void()> sub_loop=std::bind(&EventLoop::loop,subReactors[i]);
//         threadpool->add(sub_loop);
//     }
// }


// Server::~Server(){
//     delete acceptor;
//     delete threadpool;
// }


// void Server::newConnection(Socket *sock){
//     if(sock->getfd()!=-1)
//     {
//         int random=sock->getfd()%subReactors.size();
//         Connection *conn=new Connection(subReactors[random],sock);
//         std::function<void(Socket*)> cb=std::bind(&Server::deleteConnection,this,std::placeholders::_1);
//         conn->setdeleteConnectionCallback(cb);
//         connections[sock->getfd()]=conn;
//     }
// }

// void Server::deleteConnection(Socket * sock){
//     if(sock->getfd()!=-1)
//     {
//         auto it=connections.find(sock->getfd());
//         if(it!=connections.end())
//         {
//             Connection *conn=connections[sock->getfd()];
//             connections.erase(sock->getfd());
//             delete conn;
//         }
//     }
// }




#include "Server.h"
#include "Acceptor.h"
#include "Connection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "ThreadPool.h"

#include <cassert>
#include <functional>
#include <thread>

Server::Server(EventLoop *loop)
    : main_reactor_(loop), acceptor_(nullptr), thread_pool_(nullptr) {
  acceptor_ = new Acceptor(main_reactor_);
  std::function<void(Socket *)> cb =
      std::bind(&Server::NewConnection, this, std::placeholders::_1);
  acceptor_->setNewConnectionCallback(cb);

  int size = static_cast<int>(std::thread::hardware_concurrency());
  thread_pool_ = new ThreadPool(size);

  for (int i = 0; i < size; ++i) {
    sub_reactors_.push_back(new EventLoop());
  }

  for (int i = 0; i < size; ++i) {
    std::function<void()> sub_loop =
        std::bind(&EventLoop::loop, sub_reactors_[i]);
    thread_pool_->add(std::move(sub_loop));
  }
}

Server::~Server() {
  // 通知所有子Reactor退出事件循环
  for (auto sub : sub_reactors_) {
    sub->opquit();
  }
  // 销毁线程池，等待IO线程全部join结束
  delete thread_pool_;

  // 释放每一个子EventLoop对象
  for (auto sub : sub_reactors_) {
    delete sub;
  }
  sub_reactors_.clear();

  // 释放全部保存的连接对象
  for (auto &pair : connections_) {
    delete pair.second;
  }
  connections_.clear();

  delete acceptor_;
}

void Server::NewConnection(Socket *sock) {
  assert(sock->getfd() != -1);
  uint64_t idx = static_cast<uint64_t>(sock->getfd()) % sub_reactors_.size();
  Connection *conn = new Connection(sub_reactors_[idx], sock);

  std::function<void(Socket *)> cb =
      std::bind(&Server::DeleteConnection, this, std::placeholders::_1);
  conn->SetDeleteConnectionCallback(cb);

  // 将Server保存的业务回调设置给Connection
  conn->SetOnConnectCallback(on_connect_callback_);

  connections_[sock->getfd()] = conn;
}

void Server::DeleteConnection(Socket *sock) {
  int sockfd = sock->getfd();
  auto it = connections_.find(sockfd);
  if (it != connections_.end()) {
    Connection *conn = it->second;
    connections_.erase(it);
    delete conn;
    conn = nullptr;
  }
}

// 对外接口：注册业务回调，main函数调用
void Server::OnConnect(std::function<void(Connection *)> fn) {
  on_connect_callback_ = std::move(fn);
}
