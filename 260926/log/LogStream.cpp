#include <stdio.h>
#include "LogStream.h"

// ========== FixedBuffer 模板类成员函数实现 ==========
template<int SIZE>
FixedBuffer<SIZE>::FixedBuffer():cur_(data_){};

template<int SIZE>
FixedBuffer<SIZE>::~FixedBuffer(){};

template<int SIZE>
void FixedBuffer<SIZE>::append(const char *buf,int len){
    if(avail()>len){
        memcpy(cur_,buf,len);
        cur_+=len;
    }
}

template<int SIZE>
const char *FixedBuffer<SIZE>::data() const {return data_;}

template<int SIZE>
int FixedBuffer<SIZE>::len() const {return static_cast<int> (cur_ - data_);}

template<int SIZE>
char *FixedBuffer<SIZE>::current(){return cur_;}

template<int SIZE>
int FixedBuffer<SIZE>::avail()const {return static_cast<int>(end()-cur_);}

template<int SIZE>
void FixedBuffer<SIZE>::add(int len){cur_+=len;}

template<int SIZE>
void FixedBuffer<SIZE>::reset(){cur_=data_;}

template<int SIZE>
void FixedBuffer<SIZE>::clear(){
    ::bzero(data_, sizeof(data_));  // 关键：加 :: 调用全局C函数，避免和成员函数bzero重名冲突
}

template<int SIZE>
void FixedBuffer<SIZE>::bzero(){
    clear();
}

template<int SIZE>
const char *FixedBuffer<SIZE>::end()const{return data_+sizeof(data_);}

// 显式实例化两个模板版本（前端小缓冲区 + 后端大缓冲区）
template class FixedBuffer<FixedBufferSize>;
template class FixedBuffer<FixedLargeBufferSize>;

// ========== LogStream 类成员函数实现（普通类，不用改） ==========
LogStream::LogStream(){}
LogStream::~LogStream(){}

void LogStream::append(const char *data,int len){
    buffer_.append(data,len);
}

const LogStream::Buffer& LogStream::buffer()const {return buffer_;}

void LogStream::resetBuffer(){
    buffer_.clear();
    buffer_.reset();
}

LogStream& LogStream::operator<<(bool v){
    buffer_.append(v?"1":"0",1);
    return *this;
}

LogStream& LogStream::operator<<(short num){
    return (*this)<<static_cast<int>(num);
}

LogStream& LogStream::operator<<(unsigned short num){
    return (*this)<<static_cast<unsigned int>(num);
}

LogStream& LogStream:: operator<<(int num){
    formatInteger(num);
    return *this;
}

LogStream& LogStream:: operator<<(unsigned int num){
    formatInteger(num);
    return *this;
}

LogStream& LogStream:: operator<<(long num){
    formatInteger(num);
    return *this;
}

LogStream& LogStream:: operator<<(unsigned long num){
    formatInteger(num);
    return *this;
}

LogStream& LogStream:: operator<<(long long num){
    formatInteger(num);
    return *this;
}

LogStream& LogStream:: operator<<(unsigned long long num){
    formatInteger(num);
    return *this;
}

LogStream& LogStream:: operator<<(const float & num){
    return (*this) << static_cast<const double>(num);
}

LogStream& LogStream:: operator<<(const double& num){
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%g", num);
    buffer_.append(buf, len);
    return *this;
}

LogStream& LogStream:: operator<<(char v){
    buffer_.append(&v, 1);
    return *this;
}

LogStream& LogStream:: operator<<(const char* str){
    if (str){ buffer_.append(str, strlen(str)); }
    else    { buffer_.append("(null)", 6);      }
    return *this;
}

LogStream& LogStream:: operator<<(const std::string& v){
    buffer_.append(v.c_str(), v.size());
    return *this;
}
