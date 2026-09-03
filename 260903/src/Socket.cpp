#include "Socket.h"
#include "InetAddress.h"
#include "util.h"
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

Socket::Socket():fd(-1)
{
    fd=socket(AF_INET,SOCK_STREAM,0);
    errif(fd==-1,"socket create error");
}

Socket::Socket(int _fd):fd(_fd)
{
    errif(fd==-1,"socket create error");
}

Socket::~Socket()
{
    if(fd!=-1)
    {
        close(fd);
        fd=-1;
    }
}

int Socket::getfd()
{
    return fd;

}

void Socket ::setnonblocking()
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void Socket ::bind(InetAddress *_addr)
{
    struct sockaddr_in addr=_addr->getaddr();
    socklen_t addr_len=_addr->getaddr_len();
    errif(::bind(fd,(sockaddr*)&addr,addr_len)==-1,"socket bind error");
    _addr->setInetAddress(addr,addr_len);
}   

void Socket::listen()
{
    errif(::listen(fd,SOMAXCONN)==-1,"socket listen error");
}

int Socket::accept(InetAddress *_addr)
{
    struct sockaddr_in addr;
    socklen_t addr_len=sizeof(addr);
    bzero(&addr,sizeof(addr));
    int client_fd=::accept(fd,(sockaddr*)&addr,&addr_len);
    errif(client_fd==-1,"socket accept error");
    _addr->setInetAddress(addr,addr_len);
    return client_fd;
}
