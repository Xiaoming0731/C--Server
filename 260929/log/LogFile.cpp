#include "LogFile.h"
#include "TimeStamp.h"
#include <string>
#include <cstdio>

LogFile::LogFile(const char *filepath)
    : fp_(nullptr),
      written_bytes_(0),
      lastwrite_(0),
      lastflush_(0)
{
    if(filepath){
        fp_ = ::fopen(filepath, "a++");
    }
    if(!fp_){
        std::string DefaultPath = "../LogFiles/LogFile_" +
                              TimeStamp::Now().ToFormattedString() +
                              ".log";
        fp_ = ::fopen(DefaultPath.c_str(), "a+");
    }
}

LogFile::~LogFile(){
    Flush();
    if(fp_){
        fclose(fp_);
    }
}

void LogFile::Write(const char *data, int len){
    int pos = 0;
    while(pos != len){
        pos += static_cast<int>(fwrite_unlocked(data+pos, sizeof(char), len-pos, fp_));
    }
    time_t now = ::time(nullptr);
    if (len != 0){
        lastwrite_ = now;
        written_bytes_ += len;
    }
    if(lastwrite_ - lastflush_ > FlushInterval){
        Flush();
        lastflush_ = now;
    }
}

int64_t LogFile::writtenbytes() const {
    return written_bytes_;
}

void LogFile::Flush(){
    fflush(fp_);
}
