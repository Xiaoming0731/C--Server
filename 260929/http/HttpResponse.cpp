#include "HttpResponse.h"
#include <string>

HttpResponse::HttpResponse(bool close_connection) :
    status_code_(kUnKnown),
    content_length_(0),
    close_connection_(close_connection),
    filefd_(-1),
    file_offset_(0),
    body_type_(HTML_TYPE){};

HttpResponse::~HttpResponse(){};


void HttpResponse::SetStatusCode(HttpStatusCode status_code){
    status_code_ = status_code;
}

void HttpResponse::SetStatusMessage(const std::string& status_message){
    status_message_ = std::move(status_message);
}

void HttpResponse::SetCloseConnection(bool close_connection){
    close_connection_ = close_connection;
}

void HttpResponse::SetContentType(const std::string &content_type){
    AddHeader("Content-Type", content_type);
}

void HttpResponse::SetContentLength(const int &len){
    content_length_ = len;
}

int HttpResponse::GetContentLength() { return content_length_; }


void HttpResponse::AddHeader(const std::string &key, const std::string &value){
    headers_[key] = value;
}

void HttpResponse::SetBody(const std::string &body){
    body_ = body;
}

bool HttpResponse::IsCloseConnection(){
    return close_connection_;
}

std::string HttpResponse::message(){
    // 1. 未显式设置状态描述时，按状态码自动补上
    if(status_message_.empty()){
        switch(status_code_){
            case k200K:                   status_message_ = "OK"; break;
            case k206PartialContent:      status_message_ = "Partial Content"; break;
            case k302MovedTemporarily:    status_message_ = "Found"; break;
            case k400BadRequest:          status_message_ = "Bad Request"; break;
            case k403Forbidden:           status_message_ = "Forbidden"; break;
            case k404NotFound:            status_message_ = "Not Found"; break;
            case k500internalServerError: status_message_ = "Internal Server Error"; break;
            default:                      status_message_ = "OK"; break;
        }
    }
    // 2. 普通响应的 Content-Length 就是 body 的字节数
    content_length_ = static_cast<int>(body_.size());
    // 3. 复用 beforebody() 拼头部，避免两套拼头逻辑不一致
    return beforebody() + body_;
}

std::string HttpResponse::beforebody(){
    std::string message;
    message += ("HTTP/1.1 " +
                std::to_string(status_code_) + " " +
                status_message_ + "\r\n"
    );
    if(close_connection_){
        message += ("Connection: close\r\n");
    }else{
        message += ("Connection: Keep-Alive\r\n");
    }
    message += ("Content-Length: " + std::to_string(content_length_) + "\r\n");

    for (const auto&header : headers_){
        message += (header.first + ": " + header.second + "\r\n");
    }

    message += "Cache-Control: no-store, no-cache, must-revalidate\r\n";
    message += "\r\n";

    return message;
}

int HttpResponse::filefd() const { return filefd_; };
HttpResponse::HttpBodyType HttpResponse::bodytype() const { return body_type_; };
void HttpResponse::SetFileFd(int filefd) { filefd_ = filefd; };
void HttpResponse::SetBodyType(HttpBodyType bodytype) { body_type_ = bodytype; };

void HttpResponse::SetFileOffset(off_t offset){ file_offset_ = offset; }
off_t HttpResponse::fileOffset() const { return file_offset_; }
