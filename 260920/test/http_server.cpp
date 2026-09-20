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

int main(){
    // 1. 启动异步日志后端（先建目录！）
    g_async_log = std::make_unique<AsyncLogging>("../LogFiles/server.log");
    g_async_log->Start();

    // 2. 替换 Logger 输出：从 stdout → 异步文件
    Logger::setOutput(AsyncOutput);

    // 3. 启动服务器
    EventLoop loop;
    HttpServer server(&loop, "0.0.0.0", 1234);
    server.SetThreadNums(4);
    server.start();
    
    LOG_INFO << "server started";
    
    loop.Loop();
    
    // 4. 程序退出前 Stop，确保剩余日志刷完
    g_async_log->Stop();
    return 0;
}
