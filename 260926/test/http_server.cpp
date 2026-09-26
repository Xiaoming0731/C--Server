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

void DownloadFile(const std::string &filename, HttpResponse *response){
    int filefd = ::open(("../files/" + filename).c_str(), O_RDONLY);
    if(filefd == -1){
        LOG_ERROR << "OPEN FILE ERROR";
        response->SetStatusCode(HttpResponse::HttpStatusCode::k302MovedTemporarily);
        response->SetStatusMessage("Moved Temporarily");
        response->SetContentType("text/html");
        response->AddHeader("Location", "/fileserver");
    }else{
        struct stat fileStat;
        fstat(filefd, &fileStat);
        response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
        response->SetContentLength(static_cast<int>(fileStat.st_size));
        response->SetContentType("application/octet-stream");

        response->SetBodyType(HttpResponse::HttpBodyType::FILE_TYPE);
        response->SetFileFd(filefd);
    }
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
            DownloadFile(url.substr(10), response);
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
        // ========== 完整upload上传路由 ==========
        else if(url == "/upload"){
            std::string ct = request.GetHeader("Content-Type");
            if(ct.find("multipart/form-data") == std::string::npos){
                response->SetStatusCode(HttpResponse::k400BadRequest);
                response->SetBody("not upload request\n");
                return;
            }
            size_t bpos = ct.find("boundary=");
            if(bpos == std::string::npos){
                response->SetStatusCode(HttpResponse::k400BadRequest);
                response->SetBody("no boundary\n");
                return;
            }
            std::string boundary = ct.substr(bpos + 9);
            std::string msg = request.body();

            size_t fn_pos = msg.find("filename=\"");
            if(fn_pos == std::string::npos){
                LOG_ERROR << "cant find filename";
                response->SetStatusCode(HttpResponse::k400BadRequest);
                response->SetBody("parse filename fail\n");
                return;
            }
            fn_pos += 10;
            size_t fn_end = msg.find("\"\r\n", fn_pos);
            if(fn_end == std::string::npos){
                response->SetStatusCode(HttpResponse::k400BadRequest);
                response->SetBody("parse filename end fail\n");
                return;
            }
            std::string filename = msg.substr(fn_pos, fn_end - fn_pos);

            // 防路径穿越
            size_t slash = filename.find_last_of('/');
            if(slash != std::string::npos) filename = filename.substr(slash + 1);
            size_t bslash = filename.find_last_of('\\');
            if(bslash != std::string::npos) filename = filename.substr(bslash + 1);

            size_t data_begin = msg.find("\r\n\r\n");
            size_t data_end = msg.find("--" + boundary + "--");
            if(data_begin == std::string::npos || data_end == std::string::npos || (data_begin +4) >= data_end){
                response->SetStatusCode(HttpResponse::k400BadRequest);
                response->SetBody("parse upload body fail\n");
                return;
            }
            data_begin +=4;
            std::string filedata = msg.substr(data_begin, data_end - data_begin -2);

            std::ofstream ofs("../files/" + filename, std::ios::out | std::ios::trunc | std::ios::binary);
            if(!ofs.is_open()){
                LOG_ERROR << "open upload file failed:" << filename;
                response->SetStatusCode(HttpResponse::k500internalServerError);
                response->SetBody("save file fail\n");
                return;
            }
            ofs.write(filedata.data(), filedata.size());
            ofs.close();

            response->SetStatusCode(HttpResponse::HttpStatusCode::k302MovedTemporarily);
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
