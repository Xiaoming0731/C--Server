#include <iostream>
#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "EventLoop.h"
#include "Logging.h"
#include "AsyncLogging.h"
#include <string>
#include <memory>
#include <fstream>

std::string ReadFile(const std::string& path){
    std::ifstream is(path.c_str(), std::ios::binary);
    if (!is.is_open()) {
        return "";
    }

    is.seekg(0, std::ios::end);
    std::streamsize flength = is.tellg();
    is.seekg(0, std::ios::beg);

    std::string msg(flength, '\0');
    is.read(&msg[0], flength);
    return msg;
}

void HttpResponseCallback(const HttpRequest &request, HttpResponse *response)
{
    std::string url = request.url();
    if(request.method() == HttpRequest::Method::kGet){
        
        if(url == "/"){
            std::string body = ReadFile("../static/index.html");
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(body);
            response->SetContentType("text/html");
        }else if(url == "/mhw"){
            std::string body = ReadFile("../static/mhw.html");
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(body);
            response->SetContentType("text/html");
        }else if(url == "/cat.jpg"){
            std::string body = ReadFile("../static/cat.jpg");
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(body);
            response->SetContentType("image/jpeg");
        }else{
            response->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
            response->SetStatusMessage("Not Found");
            response->SetBody("Sorry Not Found\n");
            response->SetCloseConnection(true);
        }
    }
    else if( request.method() == HttpRequest::Method::kPost){
        if(url == "/login"){
            std::string rqbody = request.body();
            size_t usernamePos = rqbody.find("username=");
            size_t passwordPos = rqbody.find("password=");

            if (usernamePos == std::string::npos || passwordPos == std::string::npos) {
                response->SetStatusCode(HttpResponse::k400BadRequest);
                response->SetBody("bad request\n");
                response->SetCloseConnection(true);
                return;
            }

            usernamePos += 9;
            passwordPos += 9;

            size_t usernameEndPos = rqbody.find('&', usernamePos);
            std::string username;
            if (usernameEndPos == std::string::npos) {
                username = rqbody.substr(usernamePos);
            } else {
                username = rqbody.substr(usernamePos, usernameEndPos - usernamePos);
            }

            std::string password = rqbody.substr(passwordPos);

            if (username == "Xiaoming"){
                response->SetBody("login ok!\n");
            } else {
                response->SetBody("error!\n");
            }
            response->SetStatusCode(HttpResponse::k200K);
            response->SetContentType("text/plain");
        }
    }
    return;
}

std::unique_ptr<AsyncLogging> asynclog;
void AsyncOutputFunc(const char *data, int len)
{
    asynclog->Append(data, len);
}
void AsyncFlushFunc() {
    asynclog->Flush();
}

int main(int argc, char *argv[]){
    int port;
    if (argc <= 1)
    {
        port = 1234;
    }else if (argc == 2){
        port = atoi(argv[1]);
    }else{
        printf("error");
        exit(0);
    }

    int size = std::thread::hardware_concurrency() - 1;
    EventLoop *loop = new EventLoop();
    HttpServer *server = new HttpServer(loop, "127.0.0.1", port);
    server->SetHttpCallback(HttpResponseCallback);
    server->SetThreadNums(size);
    server->start();
    
    return 0;
}
