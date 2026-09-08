// #pragma once 
// #include <sys/epoll.h>
// #include <vector>

// class Channel;

// class Epoll
// {
//     private:
//         int epfd;
//         struct epoll_event *events;
//     public:
//     Epoll();
//     ~Epoll();

//     void updateChannel(Channel *);
//     void removeChannel(Channel *channel);
//     std::vector<Channel*> poll(int timeout=-1);
// };

#pragma once 
#include <sys/epoll.h>
#include <vector>

class Channel;

class Epoll
{
private:
    int epfd_;
    struct epoll_event *events_;
public:
    Epoll();
    ~Epoll();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    std::vector<Channel*> poll(int timeout = -1);
};
