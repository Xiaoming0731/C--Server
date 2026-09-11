#include "tcp/EventLoop.h"
#include "tcp/TcpServer.h"
#include "tcp/TcpConnection.h"
#include "tcp/Buffer.h"
#include <iostream>

int main(int argc, char *argv[]) {
    // 1. 先创建主事件循环，作为第一个参数传给 TcpServer
    EventLoop *main_loop = new EventLoop();
    TcpServer *server = new TcpServer(main_loop, "127.0.0.1", 1234);

    // 2. 回调参数必须是 const shared_ptr<TcpConnection>&，不能是裸指针
    server->set_message_callback([](const std::shared_ptr<TcpConnection>& conn) {
        std::cout << "Message from client " << conn->id() 
                  << " is " << conn->read_buf()->c_str() << std::endl;
        conn->Send(conn->read_buf()->c_str());
    });

    server->Start();

    delete server;
    delete main_loop;
    return 0;
}
