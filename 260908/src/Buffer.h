// #pragma once
// #include <string>
// #include <sys/types.h>

// class Buffer
// {
// private:
//     std::string buf;
// public:
//     Buffer() = default;
//     ~Buffer() = default;

//     // 追加len字节数据
//     void append(const char* _str, size_t len);

//     // 当前可读字节数
//     size_t readableBytes() const;

//     // 获取缓冲区起始指针，只读，不消费数据
//     const char* peek() const;

//     // 消费n字节（读完，从缓冲区移除）
//     void retrieve(size_t n);

//     // 全部消费清空
//     void retrieveAll();

//     // 把缓冲区全部转std::string返回
//     std::string retrieveAllAsString();

//     // 从fd读取数据到buffer
//     ssize_t readFd(int fd);

//     // 把buffer的数据往fd写
//     ssize_t writeFd(int fd);
// };


#pragma once
#include <string>
#include <sys/types.h>

class Buffer
{
private:
    std::string buf_;
public:
    Buffer() = default;
    ~Buffer() = default;

    void append(const char* str, size_t len);
    size_t readableBytes() const;
    const char* peek() const;
    void retrieve(size_t n);
    void retrieveAll();
    std::string retrieveAllAsString();
    ssize_t readFd(int fd);
    ssize_t writeFd(int fd);
};
