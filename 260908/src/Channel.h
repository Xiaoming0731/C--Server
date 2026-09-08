#pragma once
#include <sys/epoll.h>
#include <functional>

class EventLoop;

class Channel
{
private:
    EventLoop *loop_;
    int fd_;
    uint32_t events_;
    uint32_t revents_;
    bool inEpoll_;
    std::function<void()> readCallback_;
    std::function<void()> writeCallback_;
public:
    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent();
    void enableReading();
    void enableWriting();
    void disableWriting();
    void useET();

    int getFd();
    uint32_t getEvents();
    uint32_t getRevents();
    bool getInEpoll();
    void setInEpoll(bool in = true);
    void setReadCallback(std::function<void()> cb);
    void setWriteCallback(std::function<void()> cb);
    void setEvents(uint32_t ev);
    void setRevents(uint32_t ev);
};
