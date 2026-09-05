#pragma once
#include <functional>

class Epoll;
class Channel;
class ThreadPool;
class EventLoop
{
private:
    Epoll *ep;
    ThreadPool * threadpool;
    bool quit;
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void updateChannel(Channel*);
    //EventLoop.h
    void removeChannel(Channel* ch);
    void addthread(std::function<void()>);

};