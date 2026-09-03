#include "Connection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define READ_BUFFER 1024

Connection::Connection(EventLoop* _loop,Socket *_sock):loop(_loop),sock(_sock)
{
    channel=new Channel(loop,sock->getfd());
    std::function<void()> cb=std::bind(&Connection::echo,this,sock->getfd());
    channel->setCallback(cb);
    channel->enableReading();
}

Connection::~Connection(){
    loop->removeChannel(channel);
    delete channel;
    delete sock;
}

void Connection::echo(int sockfd){
    char buf[READ_BUFFER];
    while(true){
        bzero(&buf, sizeof(buf));
        ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
        if(bytes_read > 0){
            printf("message from client fd %d: %s\n", sockfd, buf);
            write(sockfd, buf, bytes_read); // 修复：只回写实际读到的字节
        } else if(bytes_read == -1 && errno == EINTR){
            printf("continue reading\n");
            continue;
        } else if(bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){
            printf("finish reading once, errno: %d\n", errno);
            break;
        } else if(bytes_read == 0){
            printf("EOF, client fd %d disconnected\n", sockfd);
            deleteConnectionCallback(sock);
            break;
        }
    }
}

void Connection::setdeleteConnectionCallback(std::function<void(Socket*)> _cb){
    deleteConnectionCallback = _cb;
}
