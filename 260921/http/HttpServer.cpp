#include "HttpServer.h"
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "HttpContext.h"
#include "Acceptor.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "CurrentThread.h"
#include "TimeStamp.h"
#include "Logging.h"       // 新增：引入日志库
#include <arpa/inet.h>
#include <functional>
#include <algorithm>
#include <cctype>
#include <memory>

void HttpServer::HttpDefaultCallBack(const HttpRequest& request, HttpResponse *resp){
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetCloseConnection(true);
}

HttpServer::HttpServer(EventLoop *loop, const char *ip, const int port) 
    : loop_(loop), auto_close_conn_(true)
{
    server_ = std::make_unique<TcpServer>(loop_, ip, port);
    server_->set_connection_callback(
        [this](const std::shared_ptr<TcpConnection>& conn){ onConnection(conn); });
    server_->set_message_callback(
        [this](const std::shared_ptr<TcpConnection>& conn){ onMessage(conn); });
    SetHttpCallback([this](const HttpRequest& req, HttpResponse* resp){
        HttpDefaultCallBack(req, resp);
    });
}

HttpServer::~HttpServer() {}

void HttpServer::onConnection(const TcpConnectionPtr &conn){
    int clnt_fd = conn->fd();
    struct sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    getpeername(clnt_fd, (struct sockaddr *)&peeraddr, &len);

    // 替换：std::cout → LOG_INFO，去掉重复的线程ID（日志前缀已自动携带）
    LOG_INFO << "HttpServer new connection [fd#" << clnt_fd << "] from "
             << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port);

    // 新连接建立时注册超时定时器
    if (auto_close_conn_) {
        std::weak_ptr<TcpConnection> weak_conn = conn;
        loop_->RunAfter(AUTOCLOSETIMEOUT, [this, weak_conn]() {
            ActiveCloseConn(weak_conn);
        });
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn){
    if (conn->state() != TcpConnection::ConnectionState::Connected) return;

    LOG_INFO << "=== recv size: " << conn->read_buf()->readablebytes() 
             << " data: [" << std::string(conn->read_buf()->Peek(), conn->read_buf()->readablebytes()) << "]";
    
    if (auto_close_conn_) {
        conn->UpdateTimeStamp(TimeStamp::Now());
    }

    HttpContext *context = conn->context();

    // 循环解析：处理粘包，一次读完所有完整请求
    while (true) {
        context->ParaseRequest(conn->read_buf());
        
        if (context->HasError()) {
            conn->Send("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
            conn->HandleClose();
            return;
        }

        if (!context->GetCompleteRequest()) {
            // 数据不够一个完整请求，退出循环，等下次读事件
            break;
        }

        // 处理完整请求
        onRequest(conn, *context->request());
        context->ResetContextStatus();
    }
}


void HttpServer::SetHttpCallback(const HttpServer::HttpResponseCallback &cb){
    response_callback_ = std::move(cb);
}

static std::string ToLower(std::string s){
    std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c){ return std::tolower(c); });
    return s;
}

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &request){
    std::string conn_state = ToLower(request.GetHeader("Connection"));
    bool close = (conn_state == "close" ||
                (request.version() == HttpRequest::Version::kHttp10 &&
                 conn_state != "keep-alive"));
    HttpResponse response(close);
    response_callback_(request, &response);
    conn->Send(response.message().c_str());
    if (response.IsCloseConnection()){
        conn->HandleClose();
    }
}

void HttpServer::start(){
    server_->Start();
}

void HttpServer::SetThreadNums(int thread_nums){
    server_->SetThreadNums(thread_nums);
}

// 主动关闭检查逻辑
void HttpServer::ActiveCloseConn(const std::weak_ptr<TcpConnection>& connection){
    TcpConnectionPtr conn = connection.lock();
    if (!conn) {
        return;
    }
    TimeStamp expire_time = TimeStamp::AddTime(conn->timestamp(), AUTOCLOSETIMEOUT);
    if (expire_time < TimeStamp::Now()) {
        // 替换：std::cout → LOG_INFO
        LOG_INFO << "connection [fd#" << conn->fd() << "] timeout, close";
        conn->Shutdown();
    } else {
        // 未超时，重新注册下一次检查
        std::weak_ptr<TcpConnection> weak_conn = conn;
        loop_->RunAfter(AUTOCLOSETIMEOUT, [this, weak_conn]() {
            ActiveCloseConn(weak_conn);
        });
    }
}