#include "Channel.h"
#include "EventLoop.h"
#include <unistd.h>
#include <sys/epoll.h>

Channel::Channel(EventLoop *loop, int fd) 
    : loop_(loop), fd_(fd), events_(0), revents_(0), inEpoll_(false)
{
}

Channel::~Channel(){
}

void Channel::handleEvent(){
    if(revents_ & EPOLLIN){
        if(readCallback_) readCallback_();
    }
    if(revents_ & EPOLLOUT){
        if(writeCallback_) writeCallback_();
    }
}

void Channel::useET()
{
    events_ |= EPOLLET;
    loop_->updateChannel(this);
}

void Channel::enableReading(){
    events_ |= EPOLLIN | EPOLLET;
    loop_->updateChannel(this);
}

void Channel::enableWriting(){
    events_ |= EPOLLOUT;
    loop_->updateChannel(this);
}

void Channel::disableWriting(){
    events_ &= ~EPOLLOUT;
    loop_->updateChannel(this);
}

int Channel::getFd(){
    return fd_;
}

uint32_t Channel::getEvents(){
    return events_;
}

uint32_t Channel::getRevents(){
    return revents_;
}

bool Channel::getInEpoll(){
    return inEpoll_;
}

void Channel::setInEpoll(bool in){
    inEpoll_ = in;
}

void Channel::setEvents(uint32_t ev){
    events_ = ev;
}

void Channel::setRevents(uint32_t ev){
    revents_ = ev;
}

void Channel::setReadCallback(std::function<void()> cb){
    readCallback_ = std::move(cb);
}

void Channel::setWriteCallback(std::function<void()> cb){
    writeCallback_ = std::move(cb);
}
