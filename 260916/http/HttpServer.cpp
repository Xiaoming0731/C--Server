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
#include <arpa/inet.h>
#include <functional>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <memory>

void HttpServer::HttpDefaultCallBack(const HttpRequest& request, HttpResponse *resp){
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetCloseConnection(true);
}

HttpServer::HttpServer(EventLoop *loop, const char *ip, const int port) : loop_(loop) {
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
    std::cout << CurrentThread::tid()
              << " HttpServer new connection [fd#" << clnt_fd << "] from "
              << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port)
              << std::endl;
}

void HttpServer::onMessage(const TcpConnectionPtr &conn){
    if (conn->state() != TcpConnection::ConnectionState::Connected) return;

    HttpContext *context = conn->context();
    context->ParaseRequest(conn->read_buf()->c_str(),
                           static_cast<int>(conn->read_buf()->Size()));

    if (context->HasError()){
        conn->Send("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        conn->HandleClose();
        return;
    }

    if (context->GetCompleteRequest()){
        onRequest(conn, *context->request());
        context->ResetContextStatus();
        conn->read_buf()->Clear();
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
