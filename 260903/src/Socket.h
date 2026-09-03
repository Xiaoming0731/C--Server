#pragma once
#include "InetAddress.h"


class InetAddress;

class Socket
{
    private:
        int fd;
    public:
        Socket();
        Socket(int _fd);
        ~Socket();

        int getfd();
        void bind(InetAddress *addr);
        void listen();
        void setnonblocking();

        int accept(InetAddress *addr);
};