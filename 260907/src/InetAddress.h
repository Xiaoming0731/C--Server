#pragma once
#include <arpa/inet.h>

class InetAddress
{
    private:
        struct sockaddr_in addr;
        socklen_t addr_len;
    public:
        InetAddress();
        InetAddress(const char * ip,uint16_t port);
        ~InetAddress();

        void setInetAddress(sockaddr_in _addr,socklen_t _len);
        sockaddr_in getaddr();
        socklen_t getaddr_len();

};