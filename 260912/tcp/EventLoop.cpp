#include "EventLoop.h"
#include "Channel.h"
#include "Epoller.h"
#include "CurrentThread.h"
#include "memory"
#include "vector"
#include "stdio.h"
#include "unistd.h"
#include "thread"
#include "sys/eventfd.h"
#include "assert.h"

EventLoop::EventLoop() { 
    poller_ = std::make_unique<Epoller>();
    // 创建非阻塞的eventfd，用来跨线程唤醒epoll
    wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    wakeup_channel_ = std::make_unique<Channel>(wakeup_fd_, this);
    calling_functors_ = false;
    // 注册eventfd的读回调
    wakeup_channel_->set_read_callback(std::bind(&EventLoop::HandleRead, this));
    wakeup_channel_->EnableRead();
}

EventLoop::~EventLoop() {
    DeleteChannel(wakeup_channel_.get());
    ::close(wakeup_fd_);
}

void EventLoop::Loop(){
    // 记录当前IO线程的ID
    tid_ = CurrentThread::tid();
    while (true)
    {
        // 等待IO事件
        for (Channel *active_ch : poller_->Poll()){
            active_ch->HandleEvent();
        }
        // 处理完所有IO事件后，执行待处理任务队列
        DoToDoList();
    }
}

void EventLoop::UpdateChannel(Channel *ch) { 
    poller_->UpdateChannel(ch); 
}

void EventLoop::DeleteChannel(Channel *ch) { 
    poller_->DeleteChannel(ch); 
}

bool EventLoop::IsInLoopThread(){
    return CurrentThread::tid() == tid_;
}

// 核心：本线程直接执行，其他线程就投递到队列
void EventLoop::RunOneFunc(std::function<void()> cb){
    if(IsInLoopThread()){
        cb();
    }else{
        QueueOneFunc(cb);
    }
}

// 投递任务到队列，并唤醒epoll
void EventLoop::QueueOneFunc(std::function<void()> cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        to_do_list_.emplace_back(std::move(cb));
    }
    // 如果不是本线程调用，或者正在执行任务队列，就写eventfd唤醒epoll
    if (!IsInLoopThread() || calling_functors_) {
        uint64_t write_one_byte = 1;  
        ssize_t write_size = ::write(wakeup_fd_, &write_one_byte, sizeof(write_one_byte));
        (void) write_size;
        assert(write_size == sizeof(write_one_byte));
    } 
}

// 批量执行任务队列
void EventLoop::DoToDoList(){
    calling_functors_ = true;
    std::vector<std::function<void()>> functors;
    {
        std::unique_lock<std::mutex> lock(mutex_); 
        // swap减少锁持有时间
        functors.swap(to_do_list_);
    }
    for(const auto& func : functors){
        func();
    }
    calling_functors_ = false;
}

// 读取eventfd的数据，清空可读事件
void EventLoop::HandleRead(){
    uint64_t read_one_byte = 1;
    ssize_t read_size = ::read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));
    (void) read_size;
    assert(read_size == sizeof(read_one_byte));
    return;
}
