#include "TcpConnection.h"
#include "Buffer.h"
#include "Channel.h"
#include "common.h"
#include "EventLoop.h"
#include "HttpContext.h"
#include "CurrentThread.h"
#include <memory>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <sys/socket.h>

TcpConnection::TcpConnection(EventLoop *loop, int connfd, int connid)
    : connfd_(connfd), connid_(connid), state_(ConnectionState::Invalid), loop_(loop)
{
    if (loop != nullptr)
    {
        channel_ = std::make_unique<Channel>(connfd, loop);
        channel_->EnableET();
        channel_->set_read_callback(std::bind(&TcpConnection::HandleMessage, this));
    }
    read_buf_ = std::make_unique<Buffer>();
    send_buf_ = std::make_unique<Buffer>();
    context_ = std::make_unique<HttpContext>();
}

TcpConnection::~TcpConnection(){
    ::close(connfd_);
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
    Read();
    if (on_message_)
    {
        on_message_(shared_from_this());
    }
}

EventLoop *TcpConnection::loop() const { return loop_; }
int TcpConnection::fd() const { return connfd_; }
int TcpConnection::id() const { return connid_; }
TcpConnection::ConnectionState TcpConnection::state() const { return state_; }

void TcpConnection::set_send_buf(const char *str) { send_buf_->set_buf(str); }
Buffer *TcpConnection::read_buf(){ return read_buf_.get(); }
Buffer *TcpConnection::send_buf() { return send_buf_.get(); }

void TcpConnection::Send(const std::string &msg){
    set_send_buf(msg.c_str());
    Write();
}

void TcpConnection::Send(const char *msg){
    set_send_buf(msg);
    Write();
}

void TcpConnection::Read()
{
    read_buf_->Clear();
    ReadNonBlocking();
}

void TcpConnection::Write(){
    WriteNonBlocking();
    send_buf_->Clear();
}

void TcpConnection::ReadNonBlocking(){
    char buf[1024];
    while(true){
        memset(buf, 0, sizeof(buf));
        ssize_t bytes_read = read(connfd_, buf, sizeof(buf));
        if(bytes_read > 0){
            read_buf_->Append(buf, bytes_read);
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
    int data_size = static_cast<int>(send_buf_->Size());
    const char *buf_ptr = send_buf_->c_str();
    int data_left = data_size;
    while(data_left > 0){
        ssize_t bytes_write = write(connfd_, buf_ptr + data_size - data_left, data_left);
        if(bytes_write == -1 && errno == EINTR){
            continue;
        }
        if(bytes_write == -1 && errno == EAGAIN){
            break;
        }
        if(bytes_write == -1){
            HandleClose();
            break;
        }
        data_left -= static_cast<int>(bytes_write);
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