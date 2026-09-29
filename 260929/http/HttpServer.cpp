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
#include <unistd.h>
#include <functional>
#include <algorithm>
#include <cctype>
#include <memory>

void HttpServer::HttpDefaultCallBack(const HttpRequest& request, HttpResponse *resp){
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetCloseConnection(true);
}

HttpServer::HttpServer(EventLoop * loop, const char *ip, const int port, bool auto_close_conn) : loop_(loop), auto_close_conn_(auto_close_conn) {
    server_ = std::make_unique<TcpServer>(loop_, ip, port);
    server_->set_connection_callback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));

    server_->set_message_callback(
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1)
    );
    SetHttpCallback(std::bind(&HttpServer::HttpDefaultCallBack, this, std::placeholders::_1, std::placeholders::_2));

    LOG_INFO << "HttpServer Listening on [ " << ip << ":" << port << " ]";

    //loop_->RunEvery(3.0, std::bind(&HttpServer::TestTimer_IntervalEvery3Seconds, this));
};

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

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &request){
   LOG_INFO << "HttpServer::onMessage - Request URL : " << request.url() << " from TcpConnection"
            << "[ fd#" << conn->fd() << "-id#" << conn->id() << " ]";
    std::string connection_state = request.GetHeader("Connection");
    bool isclose = (connection_state == "Close" ||
                  (request.version() == HttpRequest::Version::kHttp10 &&
                  connection_state != "keep-alive"));
    HttpResponse response(isclose);
    response_callback_(request, &response);

    // 如果是HTML，直接发送所有信息
    if(response.bodytype() == HttpResponse::HttpBodyType::HTML_TYPE){
        conn->Send(response.message());
    }else{
        // 考虑到头部字段数据量不多，直接发送完头部字段后，直接发送文件。
        conn->Send(response.beforebody());
        // 从 Range 指定的偏移开始，发送指定长度
        conn->SendFile(response.filefd(), response.GetContentLength(), response.fileOffset());
        int ret = ::close(response.filefd());
        if(ret == -1){
            LOG_ERROR << "Close File Error";
        }else{
            LOG_INFO << "Close File Ok";
        }
    }

    if(response.IsCloseConnection()){
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
