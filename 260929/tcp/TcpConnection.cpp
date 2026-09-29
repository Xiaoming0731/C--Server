#include "TcpConnection.h"
#include "Buffer.h"
#include "Channel.h"
#include "common.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "CurrentThread.h"
#include "Logging.h"
#include <memory>
#include <unistd.h>
#include <poll.h>
#include <assert.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/sendfile.h>

TcpConnection::~TcpConnection(){
    ::close(connfd_);
}

TcpConnection::TcpConnection(EventLoop *loop, int connfd, int connid): connfd_(connfd), connid_(connid), loop_(loop){

    if (loop != nullptr)
    {
        channel_ = std::make_unique<Channel>(connfd, loop);
        channel_->EnableET();
        channel_->set_read_callback(std::bind(&TcpConnection::HandleMessage, this));
        channel_->set_write_callback(std::bind(&TcpConnection::WriteNonBlocking, this));
    }
    read_buf_ = std::make_unique<Buffer>();
    send_buf_ = std::make_unique<Buffer>();
    context_ = std::make_unique<HttpContext>();
}

void TcpConnection::ConnectionEstablished(){
    state_ = ConnectionState::Connected;
    channel_->Tie(shared_from_this());
    channel_->EnableRead();
    UpdateTimeStamp(TimeStamp::Now());  // 修复：连接建立时初始化时间戳
    if (on_connect_){
        on_connect_(shared_from_this());
    }
}

void TcpConnection::HandleWrite(){

    LOG_INFO << "TcpConnection::HandlWrite";
    WriteNonBlocking();
}

void TcpConnection::ConnectionDestructor(){
    loop_->DeleteChannel(channel_.get());
}

void TcpConnection::set_connection_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn){
    on_connect_ = std::move(fn);
}

void TcpConnection::set_close_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn) {
    on_close_ = std::move(fn);
}

void TcpConnection::set_message_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn) {
    on_message_ = std::move(fn);
}

void TcpConnection::HandleClose() {
    if (state_ != ConnectionState::Disconected)
    {
        state_ = ConnectionState::Disconected;
        if(on_close_){
            on_close_(shared_from_this());
        }
    }
}

// 新增：主动关闭连接
void TcpConnection::Shutdown() {
    HandleClose();
}

void TcpConnection::HandleMessage(){
    // ReadNonBlocking 内部已边读边调用 on_message_（流式消费）
    Read();
}

EventLoop *TcpConnection::loop() const { return loop_; }
int TcpConnection::fd() const { return connfd_; }
int TcpConnection::id() const { return connid_; }
TcpConnection::ConnectionState TcpConnection::state() const { return state_; }

void TcpConnection::set_send_buf(const char *str) { send_buf_->Append(str); }
Buffer *TcpConnection::read_buf(){ return read_buf_.get(); }
Buffer *TcpConnection::send_buf(){ return send_buf_.get(); }

void TcpConnection::Send(const char *msg, int len){

    int remaining = len;
    int send_size = 0;

    // 如果此时send_buf_中没有数据，则可以先尝试发送数据，
    if (send_buf_->readablebytes() == 0){
        // 强制转换类型，方便remaining操作
        send_size = static_cast<int>(write(connfd_, msg, len));

        if(send_size >= 0){
            // 说明发送了部分数据
            remaining -= send_size;
        }else if((send_size == -1) &&
                    ((errno == EAGAIN) || (errno == EWOULDBLOCK))){
            // 说明此时TCP缓冲区是慢的，没有办法写入，什么都不做
            send_size = 0;// 说明实际上没有发送数据
        }
        else{
            LOG_ERROR << "TcpConnection::Send - TcpConnection Send ERROR";
            return;
        }
    }

    // 将剩余的数据加入到send_buf中，等待后续发送。
    assert(remaining <= len);
    if(remaining > 0){
        send_buf_->Append(msg + send_size, remaining);
        // 到达这一步时
        // 1. 还没有监听写事件，在此时进行了监听
        // 2. 监听了写事件，并且已经触发了，此时再次监听，强制触发一次，如果强制触发失败，仍然可以等待后续TCP缓冲区可写。
        channel_->EnableWrite();
    }
}

void TcpConnection::Send(const std::string &msg){
    Send(msg.data(), static_cast<int>(msg.size()));
}

void TcpConnection::Send(const char *msg){
    Send(msg, static_cast<int>(strlen(msg)));
}

void TcpConnection::Read() {
    ReadNonBlocking();
}

void TcpConnection::Write(){
    WriteNonBlocking();
}

void TcpConnection::ReadNonBlocking(){
    char buf[65536];
    while(true){
        memset(buf, 0, sizeof(buf));
        ssize_t bytes_read = read(connfd_, buf, sizeof(buf));
        if(bytes_read > 0){
            read_buf_->Append(buf, bytes_read);
            // 边读边交给业务层：流式上传会立刻写盘并 Retrieve，内存不随文件大小增长
            if(on_message_){
                on_message_(shared_from_this());
            }
            // 回调里可能已关闭连接，退出读循环
            if(state_ != ConnectionState::Connected){
                break;
            }
        }else if(bytes_read == -1 && errno == EINTR){
            continue;
        }else if((bytes_read == -1) && (
            (errno == EAGAIN) || (errno == EWOULDBLOCK))){
            break;
        }else if (bytes_read == 0){
            HandleClose();
            break;
        }else{
            HandleClose();
            break;
        }
    }
}

void TcpConnection::WriteNonBlocking(){
    while (send_buf_->readablebytes() > 0) {
        ssize_t send_size = write(connfd_, send_buf_->Peek(), send_buf_->readablebytes());

        if (send_size > 0) {
            send_buf_->Retrieve(static_cast<int>(send_size));
        } else if (send_size == -1 && errno == EINTR) {
            continue; // 被信号中断，重试
        } else if (send_size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // 内核缓冲区满了，等下一次写事件触发
            break;
        } else {
            // 发送出错，关闭连接
            LOG_ERROR << "TcpConnection::WriteNonBlocking - write error";
            HandleClose();
            return;
        }
    }

    // 数据全部发完，关闭写事件监听
    if (send_buf_->readablebytes() == 0) {
        channel_->DisableWrite();
    }
}

HttpContext *TcpConnection::context() const {
    return context_.get();
}

TimeStamp TcpConnection::timestamp() const {
    return timestamp_;
}

void TcpConnection::UpdateTimeStamp(TimeStamp now)
{
    timestamp_ = now;
}

void TcpConnection::SendFile(int filefd, int size, off_t start){
    off_t offset = start;              // 文件偏移：Range 时为请求的起始字节
    ssize_t remaining = size;          // 本次要发送的字节数

    while (remaining > 0) {
        ssize_t n = sendfile(connfd_, filefd, &offset, remaining);
        if (n > 0) {
            remaining -= n;           // 只减一次
            continue;
        }
        if (n == 0) {
            break;                    // 文件已读完
        }
        if (errno == EINTR) {
            continue;                 // 信号中断，重试
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 内核发送缓冲区满：等待连接可写后继续，保证整段/整文件发完
            struct pollfd pfd;
            pfd.fd = connfd_;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            int pr = ::poll(&pfd, 1, 30000); // 最多等 30s
            if (pr > 0 && (pfd.revents & POLLOUT)) {
                continue;
            }
            break;                    // 超时或出错
        }
        break;                         // 其他错误
    }
}
