#include "Channel.h"
#include "EventLoop.h"
#include <unistd.h>

Channel::Channel(EventLoop *_loop, int _fd) : loop(_loop), fd(_fd), events(0), revents(0), inEpoll(false){

}
Channel::~Channel(){
    // Channel只负责事件封装，不关闭fd，fd生命周期由Socket管理
}


void Channel::handleEvent(){
    callback();
}

void Channel::enableReading(){
    events |= EPOLLIN | EPOLLET;
    loop->updateChannel(this);
}

int Channel::getFd(){
    return fd;
}

uint32_t Channel::getEvents(){
    return events;
}
uint32_t Channel::getRevents(){
    return revents;
}

bool Channel::getInEpoll(){
    return inEpoll;
}

  void Channel::setInEpoll(bool _in){
        inEpoll = _in;
    }

void Channel::setEvents(uint32_t _ev){
    events = _ev;
}

void Channel::setRevents(uint32_t _ev){
    revents = _ev;
}

void Channel::setCallback(std::function<void()> _cb){
    callback = _cb;
}