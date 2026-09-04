#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "src/util.h"
#include "src/Buffer.h"
#include "src/InetAddress.h"
#include "src/Socket.h"

// // 阻塞模式下保证把 len 字节全部写入 socket
// static ssize_t writeAll(int fd, const char* buf, size_t len)
// {
//     size_t total = 0;
//     while(total < len)
//     {
//         ssize_t n = write(fd, buf + total, len - total);
//         if(n == -1)
//         {
//             if(errno == EINTR) continue;
//             return -1;
//         }
//         total += static_cast<size_t>(n);
//     }
//     return static_cast<ssize_t>(total);
// }

// // 阻塞模式下读取恰好 len 字节的数据
// static ssize_t readExact(int fd, char* buf, size_t len)
// {
//     size_t total = 0;
//     while(total < len)
//     {
//         ssize_t n = read(fd, buf + total, len - total);
//         if(n == -1)
//         {
//             if(errno == EINTR) continue;
//             return -1;
//         }
//         if(n == 0) return 0; // 对端断开连接
//         total += static_cast<size_t>(n);
//     }
//     return static_cast<ssize_t>(total);
// }

// int main() {
//     Socket* sock = new Socket();
//     InetAddress* addr = new InetAddress("127.0.0.1", 8888);
    
//     // 连接服务器，失败自动 errif 退出
//     sock->connect(addr);
//     int sockfd = sock->getfd();

//     Buffer* sendBuffer = new Buffer();
//     Buffer* readBuffer = new Buffer();

//     std::string line;
//     while(true)
//     {
//         // 1. 读取控制台输入
//         if(!std::getline(std::cin, line))
//         {
//             printf("input end, exit\n");
//             break;
//         }
//         if(line.empty()) continue;

//         // 2. 放入发送缓冲区，完整写入服务器
//         sendBuffer->append(line.c_str(), line.size());
//         ssize_t write_bytes = writeAll(sockfd, sendBuffer->peek(), sendBuffer->readableBytes());
//         errif(write_bytes == -1, "socket write error");
//         sendBuffer->retrieveAll();

//         // 3. 读取服务器回显（echo 场景：发多少收多少）
//         size_t expect = line.size();
//         char buf[1024];
//         ssize_t read_bytes = readExact(sockfd, buf, expect);
//         errif(read_bytes == -1, "socket read error");
//         if(read_bytes == 0)
//         {
//             printf("server disconnected!\n");
//             break;
//         }

//         // 4. 放入读缓冲区并打印
//         readBuffer->append(buf, static_cast<size_t>(read_bytes));
//         printf("message from server: %.*s\n", 
//                static_cast<int>(readBuffer->readableBytes()), 
//                readBuffer->peek());
//         readBuffer->retrieveAll();
//     }

//     delete readBuffer;
//     delete sendBuffer;
//     delete addr;
//     delete sock;
//     return 0;
// }



static ssize_t writeAll(int fd,const char *buf,size_t len)
{
    size_t total=0;
    while(len>total)
    {
        ssize_t n=write(fd,buf+total,len-total);
        if(n==-1)
        {
            if(errno==EINTR)continue;
            return -1;
        }
        total+=static_cast<size_t> (n);
    }
        return static_cast<size_t> (total);
}

static ssize_t readExact(int fd, char *buf,size_t len)
{
    size_t total=0;
    while(total<len)
    {
        ssize_t n=read(fd,buf+total,len-total);
        if(n==-1)
        {
            if(errno==EINTR)continue;
            return -1;
        }
            total+=static_cast<size_t>(n);
    }
            return static_cast<size_t>(total);
}


int main()
{
    Socket *sock=new Socket();
    InetAddress *addr=new InetAddress("127.0.0.1",8888);
    sock->connect(addr);
    int sockfd=sock->getfd();

    Buffer *sendBuffer=new Buffer();
    Buffer *readBuffer=new Buffer();

    std::string line;
    while(true)
    {
        if(!std::getline(std::cin,line))
        {
            printf("input end,exit\n");
            break;
        }
        if(line.empty())continue;

        sendBuffer->append(line.c_str(),line.size());
        ssize_t write_bytes=writeAll(sockfd,sendBuffer->peek(),sendBuffer->readableBytes());
        errif(write_bytes==-1,"socket write error");
        sendBuffer->retrieveAll();


        size_t expect=line.size();
        char buf[1024];
        ssize_t read_bytes=readExact(sockfd,buf,expect);
        errif(read_bytes==-1,"socket read error");
        if(read_bytes==0)
        {
            printf("server disconnected!\n");
            break;
        }

        readBuffer->append(buf,static_cast<size_t>(read_bytes));
        printf("message from server:%.*s\n",static_cast<int>(readBuffer->readableBytes()), 
               readBuffer->peek());
        readBuffer->retrieveAll();

    }

    delete readBuffer;
    delete sendBuffer;
    delete addr;
    delete sock;
    return 0;
}



