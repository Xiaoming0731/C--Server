// #pragma once
// #include <sys/epoll.h>
// #include <vector>

// class Channel;
// class Epoll
// {
// private:
//     int epfd;
//     struct epoll_event *events;
// public:
//     Epoll();
//     ~Epoll();

//     void addFd(int fd, uint32_t op);
//     void updateChannel(Channel*);
//     // std::vector<epoll_event> poll(int timeout = -1);
//     std::vector<Channel*> poll(int timeout = -1);
// };



#pragma once
#include <sys/epoll.h>
#include <vector>
#include "Channel.h"

class Chaneel;
class Epoll
{
    private:
        int epfd;
        struct epoll_event *events;
    public:
        Epoll();
        ~Epoll();

        void addFd(int fd,uint32_t op);
        std::vector<Channel*> poll(int timeout = -1);
        void updateChannel(Channel *);
};