#pragma once

class Epoll;
class Channel;
class EventLoop
{
private:
    Epoll *ep;
    bool quit;
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void updateChannel(Channel*);
    //EventLoop.h
    void removeChannel(Channel* ch);

};