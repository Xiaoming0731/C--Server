#pragma once
#include <string>
#include <map>
#include <utility>
#include <sys/types.h>


class HttpResponse{
    public:
    enum HttpStatusCode
    {
        kUnKnown = 1,
        k100Continue = 100,
        k200K = 200,
        k206PartialContent = 206,          // 断点续传：部分内容
        k302MovedTemporarily = 302,
        k400BadRequest = 400,
        k403Forbidden = 403,
        k404NotFound = 404,
        k500internalServerError = 500
    };
     enum HttpBodyType
        {
            HTML_TYPE,
            FILE_TYPE,
        };
    HttpResponse(bool close_connection);
    ~HttpResponse();

    void SetStatusCode(HttpStatusCode status_code); // 设置回应码
    void SetStatusMessage(const std::string &status_message);
    void SetCloseConnection(bool close_connection);

    void SetContentType(const std::string &content_type);
    void SetContentLength(const int &len);
    int GetContentLength();
    void AddHeader(const std::string &key, const std::string &value); // 设置回应头

    void SetBody(const std::string &body);

    std::string message(); // 将信息加入到buffer中。
    std::string beforebody();
    bool IsCloseConnection();


    int filefd() const;
    HttpBodyType bodytype() const;
    void SetFileFd(int filefd);
    void SetBodyType(HttpBodyType bodytype);

    // Range：文件起始偏移
    void SetFileOffset(off_t offset);
    off_t fileOffset() const;

    private:
        // static const std::string server_name_;

        std::map<std::string, std::string> headers_;

        HttpStatusCode status_code_;
        std::string status_message_;
        int content_length_;
        std::string body_;
        bool close_connection_;

        int filefd_;
        off_t file_offset_;        // sendfile 起始偏移
        HttpBodyType body_type_;
};
