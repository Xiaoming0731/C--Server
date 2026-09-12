#pragma once
#include "common.h"
#include "memory"
#include "mutex"
#include "functional"
#include "vector"
#include "thread"

class Epoller;
class EventLoop
{
public:
    DISALLOW_COPY_AND_MOVE(EventLoop);
    EventLoop();
    ~EventLoop();
    
    void Loop();
    void UpdateChannel(Channel *ch);
    void DeleteChannel(Channel *ch);

    // 运行队列中的所有待处理任务
    void DoToDoList();
    // 将任务加入队列，等本轮poll结束后执行
    void QueueOneFunc(std::function<void()> fn); 
    // 如果在本IO线程则直接执行，否则加入队列等待执行
    void RunOneFunc(std::function<void()> fn);
    
    // 判断当前是否运行在本EventLoop对应的IO线程
    bool IsInLoopThread();
    // eventfd读回调，用来唤醒epoll_wait
    void HandleRead();

private:
    std::unique_ptr<Epoller> poller_;
    // 待执行任务队列
    std::vector<std::function<void()>> to_do_list_;
    // 保护任务队列的互斥锁
    std::mutex mutex_;
    // 唤醒用的eventfd
    int wakeup_fd_;
    // 管理eventfd的Channel
    std::unique_ptr<Channel> wakeup_channel_;
    // 标记是否正在执行任务队列
    bool calling_functors_;
    // 记录当前EventLoop所属的IO线程ID
    pid_t tid_;
};
