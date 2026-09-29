#include <csignal>
#include <iostream>
#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "EventLoop.h"
#include "Logging.h"
#include "AsyncLogging.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

// 读取文件
std::string ReadFile(const std::string& path){
    std::ifstream is(path.c_str(), std::ios::binary | std::ios::in);
    if (!is.is_open()){
        return "";
    }
    is.seekg(0, is.end);
    int flength = static_cast<int>(is.tellg());
    if(flength <= 0) return "";
    is.seekg(0, is.beg);
    std::string msg(flength, '\0');
    is.read(&msg[0], flength);
    return msg;
}

// 获取当前目录下所有文件的名字
void FindAllFiles(const std::string& path, std::vector<std::string> &filelist){
    DIR *dir;
    struct dirent *dir_entry = NULL;
    if((dir = opendir(path.c_str())) == NULL){
        LOG_ERROR << "Opendir " << path << " failed";
        return;
    }

    while((dir_entry = readdir(dir))!= NULL){
        std::string filename = dir_entry->d_name;
        if (filename != "." && filename != ".."){
            filelist.push_back(filename);
        }
    }
}

// 构建filelist.html
std::string BuildFileHtml(){
    std::vector<std::string> filelist;
    // 以/files文件夹为例
    FindAllFiles("../files", filelist);
    // 为文件生成模板
    std::string file = "";
    for (auto filename : filelist)
    {
        file += "<tr><td>" + filename + "</td>" +
                "<td>" +
                "<a href=\"/download/" + filename + "\">下载</a>" +
                "<a href=\"/delete/" + filename + "\">删除</a>" +
                "</td></tr>" + "\n";
    }
    std::string tmp = "<!--filelist-->";
    std::string filehtml = ReadFile("../static/fileserver.html");
    if(!filehtml.empty()){
        size_t pos = filehtml.find(tmp);
        if(pos != std::string::npos){
            filehtml = filehtml.replace(pos, tmp.size(), file);
        }
    }
    return filehtml;
}

void RemoveFile(const std::string & filename){
    int ret = remove(("../files/" + filename).c_str());
    if(ret != 0){
        LOG_ERROR << "删除文件 " << filename << " 失败";
    }
}

// 断点续传下载：解析 Range 头，决定起始偏移与本次发送长度
void DownloadFile(const HttpRequest &request, const std::string &filename, HttpResponse *response){
    int filefd = ::open(("../files/" + filename).c_str(), O_RDONLY);
    if(filefd == -1){
        LOG_ERROR << "OPEN FILE ERROR";
        response->SetStatusCode(HttpResponse::HttpStatusCode::k302MovedTemporarily);
        response->SetStatusMessage("Moved Temporarily");
        response->SetContentType("text/html");
        response->AddHeader("Location", "/fileserver");
        return;
    }

    struct stat fileStat;
    fstat(filefd, &fileStat);
    long long filesize = static_cast<long long>(fileStat.st_size);

    long long start = 0;
    long long end   = filesize - 1;
    bool partial = false;

    // 形如：Range: bytes=1000-1999 或 bytes=1000-
    std::string range = request.GetHeader("Range");
    size_t bp = range.find("bytes=");
    if(bp != std::string::npos){
        std::string spec = range.substr(bp + 6);
        size_t dash = spec.find('-');
        if(dash != std::string::npos && dash > 0){
            start = std::atoll(spec.substr(0, dash).c_str());
            if(dash + 1 < spec.size()){
                end = std::atoll(spec.substr(dash + 1).c_str());
            }
            if(start < filesize){
                if(end >= filesize) end = filesize - 1; // 末尾越界则夹到文件尾
                if(start <= end) partial = true;
            }
        }
    }

    if(partial){
        // 206 Partial Content
        response->SetStatusCode(HttpResponse::HttpStatusCode::k206PartialContent);
        response->SetStatusMessage("Partial Content");
        int seg_len = static_cast<int>(end - start + 1);
        response->SetContentLength(seg_len);
        response->AddHeader("Content-Range",
            "bytes " + std::to_string(start) + "-" +
            std::to_string(end) + "/" + std::to_string(filesize));
        response->SetFileOffset(static_cast<off_t>(start));
    }else{
        // 普通整文件下载
        response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
        response->SetStatusMessage("OK");
        response->SetContentLength(static_cast<int>(filesize));
        response->SetFileOffset(0);
    }

    // 声明支持 Range，浏览器才会做续传/拖动
    response->AddHeader("Accept-Ranges", "bytes");
    response->SetContentType("application/octet-stream");
    response->SetBodyType(HttpResponse::HttpBodyType::FILE_TYPE);
    response->SetFileFd(filefd);
}

void HttpResponseCallback(const HttpRequest &request, HttpResponse *response)
{
    std::string url = request.url();
    if(request.method() == HttpRequest::Method::kGet){

        if(url == "/"){
            std::string body = ReadFile("../static/index.html");
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetContentLength(static_cast<int>(body.size()));
            response->SetBody(body);
            response->SetContentType("text/html");
        }else if(url == "/mhw"){
            std::string body = ReadFile("../static/mhw.html");
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetContentLength(static_cast<int>(body.size()));
            response->SetBody(body);
            response->SetContentType("text/html");
        }else if(url == "/cat.jpg"){
            std::string body = ReadFile("../static/cat.jpg");
            response->SetContentLength(static_cast<int>(body.size()));
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(body);
            response->SetContentType("image/jpeg");
        }else if(url == "/fileserver"){
            std::string body = BuildFileHtml();
            response->SetContentLength(static_cast<int>(body.size()));
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(body);
            response->SetContentType("text/html");
        }else if(url.substr(0, 7) == "/delete") {
            RemoveFile(url.substr(8));
            response->SetStatusCode(HttpResponse::HttpStatusCode::k302MovedTemporarily);
            response->SetStatusMessage("Moved Temporarily");
            response->SetContentType("text/html");
            response->AddHeader("Location", "/fileserver");
        }else if(url.substr(0, 9) == "/download"){
            DownloadFile(request, url.substr(10), response);
        }else if(url == "/favicon.ico"){
            std::string body = ReadFile("../static/cat.jpg");
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(body);
            response->SetContentType("image/jpeg");
        }else
        {
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
            if (usernamePos == std::string::npos || passwordPos == std::string::npos){
                response->SetStatusCode(HttpResponse::k400BadRequest);
                response->SetBody("bad request\n");
                return;
            }
            usernamePos += 9;
            passwordPos += 9;
            size_t usernameEndPos = rqbody.find('&', usernamePos);
            std::string username = (usernameEndPos == std::string::npos)
                ? rqbody.substr(usernamePos)
                : rqbody.substr(usernamePos, usernameEndPos - usernamePos);

            if (username == "Xiaoming"){
                response->SetStatusCode(HttpResponse::HttpStatusCode::k302MovedTemporarily);
                response->AddHeader("Location", "/fileserver");
            }
            else{
                response->SetBody("error!\n");
                response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
                response->SetContentType("text/plain");
            }
        }
        // ========== 流式原始上传（第1步）：文件已在解析阶段落盘，这里只负责重定向 ==========
        else if(url == "/upload_raw"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k302MovedTemporarily);
            response->SetStatusMessage("Moved Temporarily");
            response->AddHeader("Location", "/fileserver");
        }
        // ========== multipart 流式上传（第2步）：文件已边收边写盘，这里只负责重定向 ==========
        else if(url == "/upload"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k302MovedTemporarily);
            response->SetStatusMessage("Moved Temporarily");
            response->AddHeader("Location", "/fileserver");
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
    signal(SIGPIPE, SIG_IGN);
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
    HttpServer *server = new HttpServer(loop, "127.0.0.1", port, true);
    server->SetHttpCallback(HttpResponseCallback);
    server->SetThreadNums(size);
    server->start();

    return 0;
}
