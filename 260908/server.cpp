// #include "src/EventLoop.h"
// #include "src/Server.h"

// int main() {
//     EventLoop *loop = new EventLoop();
//     Server *server = new Server(loop);
//     loop->loop();
//     delete server;
//     delete loop;
//     return 0;
// }

#include "src/Server.h"
#include <iostream>
#include "src/Buffer.h"
#include "src/Connection.h"
#include "src/EventLoop.h"
#include "src/Socket.h"

int main() {
  EventLoop *loop = new EventLoop();
  Server *server = new Server(loop);
  server->OnConnect([](Connection *conn) {
    // conn->Read();
    if (conn->GetState() == Connection::State::Closed) {
      conn->Close();
      return;
    }
    const char *msg = conn->ReadBuffer();
    std::cout << "Message from client " << conn->GetSocket()->getfd() << ": " << msg << std::endl;

    conn->SetSendBuffer(msg);
    conn->Write();

    // ✅关键：使用完读缓冲区，清空！！！
    conn->GetReadBuffer()->retrieveAll();
  });

  loop->loop();
  delete server;
  delete loop;
  return 0;
}