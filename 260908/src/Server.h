// #pragma once
// #include <map>
// #include <vector>

// class EventLoop;
// class Acceptor;
// class Socket;
// class Connection;
// class ThreadPool;

// class Server
// {
//     private:
//         EventLoop *mainReactor;
//         Acceptor *acceptor;
//         std::map<int,Connection*> connections;
//         std::vector<EventLoop*> subReactors;
//         ThreadPool *threadpool;
//     public:
//         Server(EventLoop*);
//         ~Server();

//         void handleReadEvent(int);
//         void newConnection(Socket *sock);
//         void deleteConnection(Socket *sock);
// };



#pragma once
#include <functional>
#include <map>
#include <vector>

class EventLoop;
class Socket;
class Acceptor;
class Connection;
class ThreadPool;

class Server {
 private:
  EventLoop *main_reactor_;
  Acceptor *acceptor_;
  std::map<int, Connection *> connections_;
  std::vector<EventLoop *> sub_reactors_;
  ThreadPool *thread_pool_;
  std::function<void(Connection *)> on_connect_callback_;

 public:
  explicit Server(EventLoop *loop);
  ~Server();

  void NewConnection(Socket *sock);
  void DeleteConnection(Socket *sock);
  void OnConnect(std::function<void(Connection *)> fn);
};
