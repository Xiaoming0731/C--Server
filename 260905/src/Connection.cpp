#include "Connection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Buffer.h"
#include "Channel.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define READ_BUFFER 1024

Connection::Connection(EventLoop* _loop,Socket *_sock):loop(_loop),sock(_sock)
{
    channel = new Channel(loop, sock->getfd());
    // 绑定读写回调
    channel->setReadCallback(std::bind(&Connection::handleRead, this));
    channel->setWriteCallback(std::bind(&Connection::handleWrite, this));
    // 默认只开启读事件，写事件按需开启
    channel->enableReading();
}

Connection::~Connection(){
    loop->removeChannel(channel);
    delete channel;
    delete sock;
}

void Connection::handleRead(){
    int sockfd = sock->getfd();
    while(true){
        ssize_t n = inBuffer.readFd(sockfd);
        if(n > 0){
            printf("message from client fd %d, %zd bytes\n", sockfd, n);
        }
        else if(n == 0){
            // EOF 客户端断开
            printf("EOF, client fd %d disconnected\n", sockfd);
            deleteConnectionCallback(sock);
            return;
        }
        else if(n == -1){
            if(errno == EINTR) continue;
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 数据全部读完，退出循环
                break;
            }
            // 真正的错误
            perror("read error");
            deleteConnectionCallback(sock);
            return;
        }
    }

    // ========== 业务逻辑：echo 回显 ==========
    // 把输入缓冲区全部数据搬到输出缓冲区
    outBuffer.append(inBuffer.peek(), inBuffer.readableBytes());
    inBuffer.retrieveAll();

    // 尝试发送输出缓冲区
    sendOutBuffer();
}

// EPOLLOUT 触发：socket 可写，继续发送剩余数据
void Connection::handleWrite(){
    sendOutBuffer();
}

// 核心：发送输出缓冲区，管理 EPOLLOUT 开关
void Connection::sendOutBuffer(){
    if(outBuffer.readableBytes() == 0){
        // 没数据要发，关闭 EPOLLOUT，避免空触发
        channel->disableWriting();
        return;
    }

    int sockfd = sock->getfd();
    ssize_t n = outBuffer.writeFd(sockfd);

    if(n > 0){
        // writeFd 内部已经 retrieve 了已发送的字节
        if(outBuffer.readableBytes() == 0){
            // 全部发完，关闭 EPOLLOUT
            channel->disableWriting();
        }
    }
    else if(n == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            // 内核发送缓冲区满，发不出去
            // 开启 EPOLLOUT，等内核有空了触发可写事件续传
            channel->enableWriting();
        }
        else{
            perror("write error");
            deleteConnectionCallback(sock);
        }
    }
}

void Connection::setdeleteConnectionCallback(std::function<void(Socket*)> _cb){
    deleteConnectionCallback = _cb;
}
