#include "TimerQueue.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Timer.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <cstdint>

TimerQueue::TimerQueue(EventLoop *loop) 
    : loop_(loop), timerfd_(-1)
{
    CreateTimerfd();
    channel_ = std::make_unique<Channel>(timerfd_, loop);
    channel_->set_read_callback(std::bind(&TimerQueue::HandleRead, this));
    channel_->EnableRead();
}

TimerQueue::~TimerQueue() {
    if (timerfd_ != -1) {
        ::close(timerfd_);
    }
    // 清理所有未到期的定时器
    for (auto& entry : timers_) {
        delete entry.second;
    }
}

void TimerQueue::CreateTimerfd() {
    timerfd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd_ == -1) {
        perror("timerfd_create failed");
    }
}

void TimerQueue::ReadTimerFd() {
    uint64_t expirations;
    ssize_t n = ::read(timerfd_, &expirations, sizeof(expirations));
    (void)n;
}

void TimerQueue::HandleRead() {
    ReadTimerFd();
    active_timers_.clear();

    TimeStamp now = TimeStamp::Now();
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    auto end = timers_.lower_bound(sentry);

    active_timers_.insert(active_timers_.end(), timers_.begin(), end);
    timers_.erase(timers_.begin(), end);

    // 执行所有到期定时器
    for (const auto& entry : active_timers_) {
        entry.second->run();
    }

    ResetTimers();
}

void TimerQueue::ResetTimers() {
    for (auto& entry : active_timers_) {
        Timer* timer = entry.second;
        if (timer->repeat()) {
            timer->ReStart(TimeStamp::Now());
            Insert(timer);
        } else {
            delete timer;
        }
    }

    if (!timers_.empty()) {
        ResetTimerFd(timers_.begin()->second);
    }
}

bool TimerQueue::Insert(Timer *timer) {
    bool reset_instantly = false;
    if (timers_.empty() || timer->expiration() < timers_.begin()->first) {
        reset_instantly = true;
    }
    timers_.emplace(timer->expiration(), timer);
    return reset_instantly;
}

void TimerQueue::AddTimer(TimeStamp timestamp, std::function<void()> const & cb, double interval) {
    Timer* timer = new Timer(timestamp, cb, interval);
    if (Insert(timer)) {
        ResetTimerFd(timer);
    }
}

void TimerQueue::ResetTimerFd(Timer *timer) {
    struct itimerspec new_spec{};
    struct itimerspec old_spec{};

    int64_t micro_diff = timer->expiration().getsecond() - TimeStamp::Now().getsecond();
    if (micro_diff < 100) {
        micro_diff = 100;
    }

    new_spec.it_value.tv_sec = static_cast<time_t>(micro_diff / 1000000);
    new_spec.it_value.tv_nsec = static_cast<long>((micro_diff % 1000000) * 1000);

    int ret = ::timerfd_settime(timerfd_, 0, &new_spec, &old_spec);
    if (ret == -1) {
        perror("timerfd_settime failed");
    }
}
