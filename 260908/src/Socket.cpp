// #include "Socket.h"
// #include "InetAddress.h"
// #include "util.h"
// #include <sys/socket.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <stdio.h>

// Socket::Socket():fd(-1)
// {
//     fd=socket(AF_INET,SOCK_STREAM,0);
//     errif(fd==-1,"socket create error");
// }

// Socket::Socket(int _fd):fd(_fd)
// {
//     errif(fd==-1,"socket create error");
// }

// Socket::~Socket()
// {
//     if(fd!=-1)
//     {
//         close(fd);
//         fd=-1;
//     }
// }

// int Socket::getfd()
// {
//     return fd;

// }

// void Socket ::setnonblocking()
// {
//     fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
// }

// void Socket ::bind(InetAddress *_addr)
// {
//     struct sockaddr_in addr=_addr->getaddr();
//     socklen_t addr_len=_addr->getaddr_len();
//     errif(::bind(fd,(sockaddr*)&addr,addr_len)==-1,"socket bind error");
//     _addr->setInetAddress(addr,addr_len);
// }   

// void Socket::listen()
// {
//     errif(::listen(fd,SOMAXCONN)==-1,"socket listen error");
// }

// // int Socket::accept(InetAddress *_addr)
// // {
// //     struct sockaddr_in addr;
// //     socklen_t addr_len=sizeof(addr);
// //     bzero(&addr,sizeof(addr));
// //     int client_fd=::accept(fd,(sockaddr*)&addr,&addr_len);
// //     errif(client_fd==-1,"socket accept error");
// //     _addr->setInetAddress(addr,addr_len);
// //     return client_fd;
// // }

// int Socket::accept(InetAddress* _addr)
// {
//     int client_sockfd=-1;
//     struct sockaddr_in addr;
//     bzero(&addr,sizeof(addr));
//     socklen_t addr_len=sizeof(addr);
    
//     if(fcntl(fd,F_GETFL)|O_NONBLOCK){
//         while(true)
//         {
//             client_sockfd=::accept(fd,(sockaddr*)&addr,&addr_len);
//             if(client_sockfd==-1&&(errno==EAGAIN)||(errno==EWOULDBLOCK))continue;
//             else if(client_sockfd==-1){errif(true,"socket accept error");}
//             else break;
//         }
//     }
//     else 
//     {
//         client_sockfd=::accept(fd,(sockaddr*)&addr,&addr_len);
//         errif(client_sockfd==-1,"socket accept error");
//     }
//     _addr->setInetAddress(addr,addr_len);
//     return client_sockfd;
// }



// // void Socket::connect(InetAddress * _addr)
// // {
// //     struct sockaddr_in addr=_addr->getaddr();
// //     socklen_t addr_len=_addr->getaddr_len();
// //    errif(::connect(fd, (struct sockaddr*)&addr, addr_len) == -1, "socket connect error");
// // }

// void Socket::connect(InetAddress* _addr)
// {
//     struct sockaddr_in addr = _addr->getaddr();
//     if(fcntl(fd, F_GETFL) & O_NONBLOCK){
//         while(true){
//             int ret = ::connect(fd, (sockaddr*)&addr, sizeof(addr));
//             if(ret == 0){
//                 break;
//             } else if(ret == -1 && (errno == EINPROGRESS)){
//                 continue;
//             } else if(ret == -1){
//                 errif(true, "socket connect error");
//             }
//         }
//     } else{
//         errif(::connect(fd, (sockaddr*)&addr, sizeof(addr)) == -1, "socket connect error");
//     }
// }



#include "Socket.h"
#include "InetAddress.h"
#include "util.h"
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

Socket::Socket():fd_(-1)
{
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    errif(fd_ == -1, "socket create error");
}

Socket::Socket(int fd):fd_(fd)
{
    errif(fd_ == -1, "socket create error");
}

Socket::~Socket()
{
    if(fd_ != -1)
    {
        close(fd_);
        fd_ = -1;
    }
}

int Socket::getfd()
{
    return fd_;
}

void Socket::setnonblocking()
{
    fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK);
}

bool Socket::isNonBlocking(){
    int flags = fcntl(fd_, F_GETFL, 0);
    return (flags & O_NONBLOCK) != 0;
}

void Socket::bind(InetAddress *addr)
{
    struct sockaddr_in address = addr->getaddr();
    socklen_t addr_len = addr->getaddr_len();
    errif(::bind(fd_, (sockaddr*)&address, addr_len) == -1, "socket bind error");
    addr->setInetAddress(address, addr_len);
}   

void Socket::listen()
{
    errif(::listen(fd_, SOMAXCONN) == -1, "socket listen error");
}

int Socket::accept(InetAddress* addr)
{
    int client_sockfd = -1;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    socklen_t addr_len = sizeof(address);
    
    if(fcntl(fd_, F_GETFL) | O_NONBLOCK){
        while(true)
        {
            client_sockfd = ::accept(fd_, (sockaddr*)&address, &addr_len);
            if(client_sockfd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
            else if(client_sockfd == -1){ errif(true, "socket accept error");}
            else break;
        }
    }
    else 
    {
        client_sockfd = ::accept(fd_, (sockaddr*)&address, &addr_len);
        errif(client_sockfd == -1, "socket accept error");
    }
    addr->setInetAddress(address, addr_len);
    return client_sockfd;
}

void Socket::connect(InetAddress* addr)
{
    struct sockaddr_in address = addr->getaddr();
    if(fcntl(fd_, F_GETFL) & O_NONBLOCK){
        while(true){
            int ret = ::connect(fd_, (sockaddr*)&address, sizeof(address));
            if(ret == 0){
                break;
            } else if(ret == -1 && (errno == EINPROGRESS)){
                continue;
            } else if(ret == -1){
                errif(true, "socket connect error");
            }
        }
    } else{
        errif(::connect(fd_, (sockaddr*)&address, sizeof(address)) == -1, "socket connect error");
    }
}
