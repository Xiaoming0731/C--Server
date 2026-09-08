// #include "Connection.h"
// #include "EventLoop.h"
// #include "Channel.h"
// #include "Socket.h"
// #include "Buffer.h"
// #include <unistd.h>
// #include <cassert>
// #include <cstring>
// #include <cerrno>
// #include <cstdio>
// #include <iostream>

// Connection::Connection(EventLoop *loop, Socket *sock) 
//     : loop_(loop), sock_(sock), channel_(nullptr), state_(State::Invalid)
// {
//     if (loop_ != nullptr) {
//         channel_ = new Channel(loop_, sock_->getfd());
//         channel_->enableReading();
//         channel_->useET();
//     }

//     read_buffer_ = new Buffer();
//     send_buffer_ = new Buffer();
//     state_ = State::Connected;
// }

// Connection::~Connection() {
//     if (loop_ != nullptr) {
//         loop_->removeChannel(channel_);
//         delete channel_;
//     }
//     delete sock_;
//     delete read_buffer_;
//     delete send_buffer_;
// }

// void Connection::Read() {
//     assert(state_ == State::Connected);
//     read_buffer_->retrieveAll();  // 对应教程的 clear()
//     ReadNonBlocking();
// }

// void Connection::Write() {
//     assert(state_ == State::Connected);
//     WriteNonBlocking();
//     send_buffer_->retrieveAll();  // 对应教程的 clear()
// }

// void Connection::ReadNonBlocking() {
//     int sockfd = sock_->getfd();
//     char buf[1024];
//     while (true) {
//         memset(buf, 0, sizeof(buf));
//         ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
//         if (bytes_read > 0) {
//             read_buffer_->append(buf, static_cast<size_t>(bytes_read));
//         } 
//         else if (bytes_read == -1 && errno == EINTR) {
//             continue;
//         } 
//         else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
//             break;
//         } 
//         else if (bytes_read == 0) {
//             printf("read EOF, client fd %d disconnected\n", sockfd);
//             state_ = State::Closed;
//             break;
//         } 
//         else {
//             printf("Other error on client fd %d\n", sockfd);
//             state_ = State::Closed;
//             break;
//         }
//     }
// }

// void Connection::WriteNonBlocking() {
//     int sockfd = sock_->getfd();
//     const char *data = send_buffer_->peek();
//     size_t data_left = send_buffer_->readableBytes();

//     while (data_left > 0) {
//         ssize_t bytes_write = write(sockfd, data + (send_buffer_->readableBytes() - data_left), data_left);
//         if (bytes_write == -1 && errno == EINTR) {
//             continue;
//         }
//         if (bytes_write == -1 && errno == EAGAIN) {
//             break;
//         }
//         if (bytes_write == -1) {
//             printf("Other error on client fd %d\n", sockfd);
//             state_ = State::Closed;
//             break;
//         }
//         data_left -= static_cast<size_t>(bytes_write);
//     }
// }

// void Connection::Close() {
//     if (delete_connectioin_callback_) {
//         delete_connectioin_callback_(sock_);
//     }
// }

// Connection::State Connection::GetState() {
//     return state_;
// }

// void Connection::SetSendBuffer(const char *str) {
//     send_buffer_->retrieveAll();
//     send_buffer_->append(str, strlen(str));
// }

// Buffer *Connection::GetReadBuffer() {
//     return read_buffer_;
// }

// const char *Connection::ReadBuffer() {
//     return read_buffer_->peek();
// }

// Buffer *Connection::GetSendBuffer() {
//     return send_buffer_;
// }

// const char *Connection::SendBuffer() {
//     return send_buffer_->peek();
// }

// void Connection::SetDeleteConnectionCallback(std::function<void(Socket *)> const &callback) {
//     delete_connectioin_callback_ = callback;
// }

// // 核心：业务回调注入，绑定到读事件
// void Connection::SetOnConnectCallback(std::function<void(Connection *)> const &callback) {
//     on_connect_callback_ = callback;
//     if (channel_) {
//         channel_->setReadCallback([this]() {
//             // 先读数据到缓冲区，再调用业务回调
//             ReadNonBlocking();
//             if (on_connect_callback_) {
//                 on_connect_callback_(this);
//             }
//         });
//     }
// }

// void Connection::GetlineSendBuffer() {
//     // 从标准输入读取一行到发送缓冲区
//     std::string line;
//     std::getline(std::cin, line);
//     send_buffer_->retrieveAll();
//     send_buffer_->append(line.c_str(), line.size());
// }

// Socket *Connection::GetSocket() {
//     return sock_;
// }




#include "Connection.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
#include "Buffer.h"
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <iostream>

Connection::Connection(EventLoop *loop, Socket *sock)
    : loop_(loop), sock_(sock), channel_(nullptr), state_(State::Invalid)
{
    if (loop_ != nullptr) {
        channel_ = new Channel(loop_, sock_->getfd());
        channel_->enableReading();
        channel_->useET();
    }

    read_buffer_ = new Buffer();
    send_buffer_ = new Buffer();
    state_ = State::Connected;
}

Connection::~Connection() {
    if (loop_ != nullptr) {
        loop_->removeChannel(channel_);
        delete channel_;
    }
    delete sock_;
    delete read_buffer_;
    delete send_buffer_;
}

void Connection::Read() {
    assert(state_ == State::Connected);
    read_buffer_->retrieveAll();
    ReadNonBlocking();
}

void Connection::Write() {
    assert(state_ == State::Connected);
    WriteNonBlocking();
    send_buffer_->retrieveAll();
}

void Connection::ReadNonBlocking() {
    int sockfd = sock_->getfd();
    char buf[1024];
    while (true) {
        memset(buf, 0, sizeof(buf));
        ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
        if (bytes_read > 0) {
            read_buffer_->append(buf, static_cast<size_t>(bytes_read));
        }
        else if (bytes_read == -1 && errno == EINTR) {
            continue;
        }
        else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        else if (bytes_read == 0) {
            printf("read EOF, client fd %d disconnected\n", sockfd);
            state_ = State::Closed;
            break;
        }
        else {
            printf("Other error on client fd %d\n", sockfd);
            state_ = State::Closed;
            break;
        }
    }
}

void Connection::WriteNonBlocking() {
    int sockfd = sock_->getfd();
    const char *data = send_buffer_->peek();
    size_t data_left = send_buffer_->readableBytes();

    while (data_left > 0) {
        ssize_t bytes_write = write(sockfd, data + (send_buffer_->readableBytes() - data_left), data_left);
        if (bytes_write == -1 && errno == EINTR) {
            continue;
        }
        if (bytes_write == -1 && errno == EAGAIN) {
            break;
        }
        if (bytes_write == -1) {
            printf("Other error on client fd %d\n", sockfd);
            state_ = State::Closed;
            break;
        }
        data_left -= static_cast<size_t>(bytes_write);
    }
}

void Connection::Close() {
    if (delete_connection_callback_) {
        delete_connection_callback_(sock_);
    }
}

Connection::State Connection::GetState() {
    return state_;
}

void Connection::SetSendBuffer(const char *str) {
    send_buffer_->retrieveAll();
    send_buffer_->append(str, strlen(str));
}

Buffer *Connection::GetReadBuffer() {
    return read_buffer_;
}

const char *Connection::ReadBuffer() {
    return read_buffer_->peek();
}

Buffer *Connection::GetSendBuffer() {
    return send_buffer_;
}

const char *Connection::SendBuffer() {
    return send_buffer_->peek();
}

void Connection::SetDeleteConnectionCallback(std::function<void(Socket *)> const &callback) {
    delete_connection_callback_ = callback;
}

void Connection::SetOnConnectCallback(std::function<void(Connection *)> const &callback) {
    on_connect_callback_ = callback;
    if (channel_) {
        channel_->setReadCallback([this]() {
            ReadNonBlocking();
            if (on_connect_callback_) {
                on_connect_callback_(this);
            }
        });
    }
}

void Connection::GetlineSendBuffer() {
    std::string line;
    std::getline(std::cin, line);
    send_buffer_->retrieveAll();
    send_buffer_->append(line.c_str(), line.size());
}

Socket *Connection::GetSocket() {
    return sock_;
}
