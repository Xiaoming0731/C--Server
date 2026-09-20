#pragma once
#include <functional>
#include <memory>
#include <stdio.h>
#include "common.h"

#define AUTOCLOSETIMEOUT 60

class TimeStamp;
class TcpServer;
class TcpConnection;
class HttpRequest;
class HttpResponse;
class EventLoop;

class HttpServer
{
public:
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void(const HttpRequest &, HttpResponse *)> HttpResponseCallback;

    DISALLOW_COPY_AND_MOVE(HttpServer);
    HttpServer(EventLoop *loop, const char *ip, const int port);
    ~HttpServer();

    void HttpDefaultCallBack(const HttpRequest &request, HttpResponse *resp);
    void SetHttpCallback(const HttpResponseCallback &cb);
    void start();

    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn);
    void onRequest(const TcpConnectionPtr &conn, const HttpRequest &request);

    void SetThreadNums(int thread_nums);

    void ActiveCloseConn(const std::weak_ptr<TcpConnection>& connection);  // 修复：改为私有+const引用
private:
    EventLoop *loop_;
    std::unique_ptr<TcpServer> server_;
    bool auto_close_conn_;
    HttpResponseCallback response_callback_;
};