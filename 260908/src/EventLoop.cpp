// #include "EventLoop.h"
// #include "Epoll.h"
// #include "Channel.h"
// #include "ThreadPool.h"
// #include "util.h"
// #include <vector>
// #include <unistd.h>
// #include <fcntl.h>

// EventLoop::EventLoop() 
//     : ep(nullptr), threadpool(nullptr), quit(false),
//       wakeupFd{-1, -1}, wakeupChannel(nullptr)
// {
//     ep = new Epoll();
//     threadpool = new ThreadPool();

//     // 记录 IO 线程 ID（创建 EventLoop 的线程就是 IO 线程）
//     loopThreadId = std::this_thread::get_id();

//     // 创建唤醒管道
//     errif(pipe(wakeupFd) == -1, "create wakeup pipe error");

//     // 设置管道读端为非阻塞
//     int flags = fcntl(wakeupFd[0], F_GETFL, 0);
//     fcntl(wakeupFd[0], F_SETFL, flags | O_NONBLOCK);

//     // 用 Channel 管理管道读端，注册到 epoll
//     wakeupChannel = new Channel(this, wakeupFd[0]);
//     wakeupChannel->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
//     wakeupChannel->enableReading();
// }

// EventLoop::~EventLoop()
// {
//     delete wakeupChannel;
//     if(wakeupFd[0] != -1) close(wakeupFd[0]);
//     if(wakeupFd[1] != -1) close(wakeupFd[1]);

//     delete ep;
//     delete threadpool;
// }

// void EventLoop::loop()
// {
//     while(!quit)
//     {
//         std::vector<Channel*> chs = ep->poll();
//         for(auto it = chs.begin(); it != chs.end(); ++it)
//         {
//             (*it)->handleEvent();
//         }

//         // 处理完所有 IO 事件后，执行跨线程投递的回调
//         doPendingFunctors();
//     }
// }

// void EventLoop::updateChannel(Channel *ch)
// {
//     ep->updateChannel(ch);
// }

// void EventLoop::removeChannel(Channel* ch)
// {
//     ep->removeChannel(ch);
// }

// void EventLoop::addthread(std::function<void()> func)
// {
//     threadpool->add(func);
// }
// void EventLoop::opquit()
// {
//     quit = true;
//     wakeup(); // 唤醒epoll_wait，让循环立刻检测quit退出
// }

// // ========== runInLoop 核心实现 ==========
// void EventLoop::runInLoop(std::function<void()> cb)
// {
//     if(isInLoopThread())
//     {
//         // 在 IO 线程里，直接执行
//         cb();
//     }
//     else
//     {
//         // 在其他线程，排队 + 唤醒
//         {
//             std::lock_guard<std::mutex> lock(mutex_);
//             pendingFunctors.push_back(std::move(cb));
//         }
//         wakeup();
//     }
// }

// bool EventLoop::isInLoopThread() const
// {
//     return std::this_thread::get_id() == loopThreadId;
// }

// void EventLoop::wakeup()
// {
//     char one = 'x';
//     ssize_t n = write(wakeupFd[1], &one, sizeof(one));
//     // 写失败不用处理，只是唤醒信号
//     (void)n;
// }

// void EventLoop::handleWakeup()
// {
//     // 把管道里所有字节都读出来，清空管道
//     char buf[64];
//     while(true)
//     {
//         ssize_t n = read(wakeupFd[0], buf, sizeof(buf));
//         if(n > 0) continue;
//         if(n == -1)
//         {
//             if(errno == EAGAIN || errno == EWOULDBLOCK) break;
//             break;
//         }
//         break;
//     }
// }

// void EventLoop::doPendingFunctors()
// {
//     std::vector<std::function<void()>> functors;

//     // 用 swap 减少锁持有时间
//     {
//         std::lock_guard<std::mutex> lock(mutex_);
//         functors.swap(pendingFunctors);
//     }

//     // 无锁状态下执行所有回调
//     for(auto &cb : functors)
//     {
//         cb();
//     }
// }




#include "EventLoop.h"
#include "Epoll.h"
#include "Channel.h"
#include "ThreadPool.h"
#include "util.h"
#include <vector>
#include <unistd.h>
#include <fcntl.h>

EventLoop::EventLoop() 
    : ep_(nullptr), threadpool_(nullptr), quit_(false),
      wakeupFd_{-1, -1}, wakeupChannel_(nullptr)
{
    ep_ = new Epoll();
    threadpool_ = new ThreadPool();
    loopThreadId_ = std::this_thread::get_id();

    errif(pipe(wakeupFd_) == -1, "create wakeup pipe error");
    int flags = fcntl(wakeupFd_[0], F_GETFL, 0);
    fcntl(wakeupFd_[0], F_SETFL, flags | O_NONBLOCK);

    wakeupChannel_ = new Channel(this, wakeupFd_[0]);
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    delete wakeupChannel_;
    if(wakeupFd_[0] != -1) close(wakeupFd_[0]);
    if(wakeupFd_[1] != -1) close(wakeupFd_[1]);
    delete ep_;
    delete threadpool_;
}

void EventLoop::loop()
{
    while(!quit_)
    {
        std::vector<Channel*> chs = ep_->poll();
        for(auto it = chs.begin(); it != chs.end(); ++it)
        {
            (*it)->handleEvent();
        }
        doPendingFunctors();
    }
}

void EventLoop::updateChannel(Channel *ch)
{
    ep_->updateChannel(ch);
}

void EventLoop::removeChannel(Channel* ch)
{
    ep_->removeChannel(ch);
}

void EventLoop::addthread(std::function<void()> func)
{
    threadpool_->add(func);
}

void EventLoop::opquit()
{
    quit_ = true;
    wakeup();
}

void EventLoop::runInLoop(std::function<void()> cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingFunctors_.push_back(std::move(cb));
        }
        wakeup();
    }
}

bool EventLoop::isInLoopThread() const
{
    return std::this_thread::get_id() == loopThreadId_;
}

void EventLoop::wakeup()
{
    char one = 'x';
    ssize_t n = write(wakeupFd_[1], &one, sizeof(one));
    (void)n;
}

void EventLoop::handleWakeup()
{
    char buf[64];
    while(true)
    {
        ssize_t n = read(wakeupFd_[0], buf, sizeof(buf));
        if(n > 0) continue;
        if(n == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
            break;
        }
        break;
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<std::function<void()>> functors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(auto &cb : functors)
    {
        cb();
    }
}
