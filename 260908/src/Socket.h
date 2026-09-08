// #pragma once
// #include "InetAddress.h"


// class InetAddress;

// class Socket
// {
//     private:
//         int fd;
//     public:
//         Socket();
//         Socket(int _fd);
//         ~Socket();

//         int getfd();
//         void bind(InetAddress *addr);
//         void listen();
//         void setnonblocking();

//         int accept(InetAddress *addr);
//         void connect(InetAddress * addr);
// };


#pragma once
#include "InetAddress.h"

class InetAddress;

class Socket
{
private:
    int fd_;
public:
    Socket();
    Socket(int fd);
    ~Socket();
    int getfd();
    void bind(InetAddress *addr);
    void listen();
    void setnonblocking();
    int accept(InetAddress *addr);
    void connect(InetAddress * addr);
    bool isNonBlocking();
};
