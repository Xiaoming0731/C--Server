#pragma once 
#include <stdio.h>
#include <ctime>
#include <string>
#include "TimeStamp.h"

// 把 time_t 换成 int，秒级间隔用整数足够
static const int FlushInterval = 3;

class LogFile{
public:
    LogFile(const char *filepath=nullptr);
    ~LogFile();
    void Write(const char *data,int len);
    void Flush();
    
    int64_t writtenbytes() const;
private:
    FILE *fp_;
    int64_t written_bytes_;
    time_t lastwrite_;
    time_t lastflush_;
};
