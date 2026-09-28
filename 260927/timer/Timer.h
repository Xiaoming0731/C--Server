#pragma once
#include <functional>
#include "common.h"
#include "TimeStamp.h"
class TimeStamp;
class Timer
{
    private:
    TimeStamp expiration_;
    std::function<void()> callback_;
    double interval_;
    bool repeat_;
    
    public:
    DISALLOW_COPY_AND_MOVE(Timer);
    Timer(TimeStamp timestamp,std::function<void()>const &cb,double interval);
    void ReStart(TimeStamp now);
    void run()const;
    TimeStamp expiration() const;
    bool repeat() const;
};