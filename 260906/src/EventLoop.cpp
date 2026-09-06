#include "EventLoop.h"
#include "Epoll.h"
#include "Channel.h"
#include "ThreadPool.h"
#include "util.h"
#include <vector>
#include <unistd.h>
#include <fcntl.h>

EventLoop::EventLoop() 
    : ep(nullptr), threadpool(nullptr), quit(false),
      wakeupFd{-1, -1}, wakeupChannel(nullptr)
{
    ep = new Epoll();
    threadpool = new ThreadPool();

    // 记录 IO 线程 ID（创建 EventLoop 的线程就是 IO 线程）
    loopThreadId = std::this_thread::get_id();

    // 创建唤醒管道
    errif(pipe(wakeupFd) == -1, "create wakeup pipe error");

    // 设置管道读端为非阻塞
    int flags = fcntl(wakeupFd[0], F_GETFL, 0);
    fcntl(wakeupFd[0], F_SETFL, flags | O_NONBLOCK);

    // 用 Channel 管理管道读端，注册到 epoll
    wakeupChannel = new Channel(this, wakeupFd[0]);
    wakeupChannel->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
    wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    delete wakeupChannel;
    if(wakeupFd[0] != -1) close(wakeupFd[0]);
    if(wakeupFd[1] != -1) close(wakeupFd[1]);

    delete ep;
    delete threadpool;
}

void EventLoop::loop()
{
    while(!quit)
    {
        std::vector<Channel*> chs = ep->poll();
        for(auto it = chs.begin(); it != chs.end(); ++it)
        {
            (*it)->handleEvent();
        }

        // 处理完所有 IO 事件后，执行跨线程投递的回调
        doPendingFunctors();
    }
}

void EventLoop::updateChannel(Channel *ch)
{
    ep->updateChannel(ch);
}

void EventLoop::removeChannel(Channel* ch)
{
    ep->removeChannel(ch);
}

void EventLoop::addthread(std::function<void()> func)
{
    threadpool->add(func);
}

// ========== runInLoop 核心实现 ==========
void EventLoop::runInLoop(std::function<void()> cb)
{
    if(isInLoopThread())
    {
        // 在 IO 线程里，直接执行
        cb();
    }
    else
    {
        // 在其他线程，排队 + 唤醒
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingFunctors.push_back(std::move(cb));
        }
        wakeup();
    }
}

bool EventLoop::isInLoopThread() const
{
    return std::this_thread::get_id() == loopThreadId;
}

void EventLoop::wakeup()
{
    char one = 'x';
    ssize_t n = write(wakeupFd[1], &one, sizeof(one));
    // 写失败不用处理，只是唤醒信号
    (void)n;
}

void EventLoop::handleWakeup()
{
    // 把管道里所有字节都读出来，清空管道
    char buf[64];
    while(true)
    {
        ssize_t n = read(wakeupFd[0], buf, sizeof(buf));
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

    // 用 swap 减少锁持有时间
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors);
    }

    // 无锁状态下执行所有回调
    for(auto &cb : functors)
    {
        cb();
    }
}
