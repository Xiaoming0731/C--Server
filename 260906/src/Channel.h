#pragma once
#include <sys/epoll.h>
#include <functional>
class EventLoop;
class Channel
{
private:
    EventLoop *loop;
    int fd;
    uint32_t events;
    uint32_t revents;
    bool inEpoll;
   std::function<void()> readCallback;   // 读事件回调
    std::function<void()> writeCallback;  // 写事件回调
public:
    Channel(EventLoop *_loop, int _fd);
    ~Channel();

    void handleEvent();
    void enableReading();
    void enableWriting();  // 开启 EPOLLOUT
    void disableWriting(); // 关闭 EPOLLOUT

    int getFd();
    uint32_t getEvents();
    uint32_t getRevents();
    bool getInEpoll();
    // Channel.h
    void setInEpoll(bool _in = true);

    // Channel.cpp
    void setReadCallback(std::function<void()> _cb);
    void setWriteCallback(std::function<void()> _cb);


    void setEvents(uint32_t);
    void setRevents(uint32_t);
    void setCallback(std::function<void()>);
};