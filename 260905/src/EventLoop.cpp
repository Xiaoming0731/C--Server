#include "EventLoop.h"
#include "Epoll.h"
#include "Channel.h"
#include "ThreadPool.h"
#include <vector>

EventLoop::EventLoop() : ep(nullptr),threadpool(nullptr),quit(false){
    ep = new Epoll();
    threadpool= new ThreadPool();
}

EventLoop::~EventLoop()
{
    delete ep;
    delete threadpool;
}


void EventLoop::loop(){
    while(!quit){
    std::vector<Channel*> chs;
        chs = ep->poll();
        for(auto it = chs.begin(); it != chs.end(); ++it){
            (*it)->handleEvent();
        }
    }
}

void EventLoop::updateChannel(Channel *ch){
    ep->updateChannel(ch);
}

void EventLoop::removeChannel(Channel* ch){
    ep->removeChannel(ch);
}

void EventLoop::addthread(std::function<void()> func)
{
    threadpool->add(func);
}