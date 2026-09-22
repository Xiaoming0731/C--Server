#include <iostream>
#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "EventLoop.h"
#include <string>

#include "HttpServer.h"
#include "AsyncLogging.h"
#include <memory>

#include <thread>

const std::string html = " <font color=\"red\">This is html!</font> ";
void HttpResponseCallback(const HttpRequest &request, HttpResponse *response)
{
    if(request.method() != HttpRequest::Method::kGet){
        response->SetStatusCode(HttpResponse::HttpStatusCode::k400BadRequest);
        response->SetStatusMessage("Bad Request");
        response->SetCloseConnection(true);
        return ;
    }

    {
        std::string url = request.url();
        if(url == "/"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(html);
            response->SetContentType("text/html");
        }else if(url == "/hello"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody("hello world\n");
            response->SetContentType("text/plain");
        }else if(url == "/favicon.ico"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
        }else{
            response->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
            response->SetStatusMessage("Not Found");
            response->SetBody("Sorry Not Found\n");
            response->SetCloseConnection(true);
        }
    }
    return;
}
// 全局异步日志实例
std::unique_ptr<AsyncLogging> g_async_log;

// 桥接函数：把 Logger 的输出路由到 AsyncLogging
void AsyncOutput(const char* data, int len) {
    g_async_log->Append(data, len);
}

int main() {
    EventLoop loop;
    HttpServer server(&loop, "0.0.0.0", 1234);

    // 新增：自定义 HTTP 回调，处理 /hello 路径
    server.SetHttpCallback([](const HttpRequest& req, HttpResponse* resp) {
        if (req.url() == "/hello") {
            resp->SetStatusCode(HttpResponse::k200K);
            resp->SetContentType("text/plain");
            resp->SetBody("Hello, Server!");
            resp->SetCloseConnection(true);
        } else {
            resp->SetStatusCode(HttpResponse::k404NotFound);
            resp->SetStatusMessage("Not Found");
            resp->SetCloseConnection(true);
        }
    });

    server.start();
    return 0;
}