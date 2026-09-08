// #include "Buffer.h"
// #include <unistd.h>

// void Buffer::append(const char *_str, size_t len)
// {
//     buf.append(_str, len);
// }

// size_t Buffer::readableBytes() const
// {
//     return buf.size();
// }

// const char* Buffer::peek() const
// {
//     return buf.data();
// }

// void Buffer::retrieve(size_t n)
// {
//     if(n >= buf.size())
//     {
//         retrieveAll();
//     }
//     else
//     {
//         buf.erase(0, n);
//     }
// }

// void Buffer::retrieveAll()
// {
//     buf.clear();
// }

// std::string Buffer::retrieveAllAsString()
// {
//     std::string res = std::move(buf);
//     retrieveAll();
//     return res;
// }

// ssize_t Buffer::readFd(int fd)
// {
//     char temp[1024];
//     ssize_t n = read(fd, temp, sizeof temp);
//     if(n > 0)
//     {
//         append(temp, static_cast<size_t>(n));
//     }
//     return n;
// }

// ssize_t Buffer::writeFd(int fd)
// {
//     ssize_t n = write(fd, peek(), readableBytes());
//     if(n > 0)
//     {
//         retrieve(static_cast<size_t>(n));
//     }
//     return n;
// }


#include "Buffer.h"
#include <unistd.h>

void Buffer::append(const char* str, size_t len)
{
    buf_.append(str, len);
}

size_t Buffer::readableBytes() const
{
    return buf_.size();
}

const char* Buffer::peek() const
{
    return buf_.data();
}

void Buffer::retrieve(size_t n)
{
    if(n >= buf_.size())
    {
        retrieveAll();
    }
    else
    {
        buf_.erase(0, n);
    }
}

void Buffer::retrieveAll()
{
    buf_.clear();
}

std::string Buffer::retrieveAllAsString()
{
    std::string res = std::move(buf_);
    retrieveAll();
    return res;
}

ssize_t Buffer::readFd(int fd)
{
    char temp[1024];
    ssize_t n = read(fd, temp, sizeof temp);
    if(n > 0)
    {
        append(temp, static_cast<size_t>(n));
    }
    return n;
}

ssize_t Buffer::writeFd(int fd)
{
    ssize_t n = write(fd, peek(), readableBytes());
    if(n > 0)
    {
        retrieve(static_cast<size_t>(n));
    }
    return n;
}
