#pragma once
#include <functional>
#include <vector>
#include <mutex>
#include <thread>

class Epoll;
class Channel;
class ThreadPool;

class EventLoop
{
private:
    Epoll *ep;
    ThreadPool * threadpool;
    bool quit;

    // ========== runInLoop 新增成员 ==========
    int wakeupFd[2];          // 唤醒管道：[0]读端 [1]写端
    Channel *wakeupChannel;   // 监听管道读端的 Channel
    std::vector<std::function<void()>> pendingFunctors; // 待执行回调队列
    std::mutex mutex_;        // 保护 pendingFunctors
    std::thread::id loopThreadId; // 记录 IO 线程 ID

    void handleWakeup();      // 管道可读事件回调
    void doPendingFunctors(); // 执行所有待执行回调
    void wakeup();            // 往管道写字节，唤醒 IO 线程

public:
    EventLoop();
    ~EventLoop();
    void loop();
    void updateChannel(Channel*);
    void removeChannel(Channel* ch);
    void addthread(std::function<void()>);
    void opquit();

    // ========== runInLoop 核心接口 ==========
    void runInLoop(std::function<void()> cb);
    bool isInLoopThread() const;
};
