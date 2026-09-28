#pragma once
#include <string>
#include <Buffer.h>
#include <memory>
class HttpRequest;


#define CR '\r'
#define LF '\n'

enum HttpRequestParaseState
{
    kINVALID,         // 无效
    kINVALID_METHOD,  // 无效请求方法
    kINVALID_URL,     // 无效请求路径
    kINVALID_VERSION, // 无效的协议版本号
    kINVALID_HEADER,  // 无效请求头

    START,  // 解析开始
    METHOD, // 请求方法

    BEFORE_URL, // 请求连接前的状态，需要'/'开头
    IN_URL,     // url处理

    BEFORE_URL_PARAM_KEY, // URL请求参数键之前
    URL_PARAM_KEY, // URL请求参数键
    BEFORE_URL_PARAM_VALUE, // URL请求参数值之前
    URL_PARAM_VALUE, // URL请求参数值

    BEFORE_PROTOCOL, // 协议解析之前
    PROTOCOL,        // 协议

    BEFORE_VERSION, // 版本开始前
    VERSION_SPLIT,  // 版本分割符
    VERSION,        // 版本

    HEADER,
    HEADER_KEY, //

    HEADER_BEFORE_COLON, // 请求头冒号之前
    HEADER_AFTER_COLON,  // 请求头冒号
    HEADER_VALUE,        // 请求值

    WHEN_CR, // 遇到一个回车

    CR_LF, // 回车换行

    CR_LF_CR, // 回车换行之后的状态

    BODY, // 请求体

    STREAM_UPLOAD, // 流式上传：body 边收边写盘，不进内存
    MP_UPLOAD,     // multipart 流式上传：边解析边写盘

    COMPLETE, // 完成

};

class HttpContext
{

public:

    // 状态转换机，保存解析的状态

    HttpContext();
    ~HttpContext();

    bool ParaseRequest(const char *begin, int size); 
    bool ParaseRequest(const std::string& msg);
    bool ParaseRequest(const char *begin);
    bool ParaseRequest(Buffer *buffer);
    bool GetCompleteRequest();
    HttpRequest *request();
    void ResetContextStatus();
    bool HasError() const;

private:
    // —— 流式上传（第1步：原始字节，/upload_raw?filename=）——
    bool BeginStreamUpload();           // 解析文件名、打开 .part 临时文件
    bool StreamUploadToFile(Buffer *buffer); // 把 buffer 里的 body 写入文件，到齐返回 true

    // —— multipart 流式上传（第2步，/upload）——
    bool BeginMultipartUpload();        // 取 boundary，进入 multipart 流程
    bool ProcessMultipart(Buffer *buffer); // 边解析 preamble/filename 边写文件内容
    bool MpWriteToFile(const char *data, size_t len); // 写盘（处理短写）
    bool MpFinalize();                  // close + 原子改名

    std::unique_ptr<HttpRequest> request_;
    HttpRequestParaseState state_;
    int parsed_len_;

    int upload_fd_;                 // 正在写入的临时文件 fd
    long long upload_remaining_;    // 还剩多少字节没写（仅第1步用）
    std::string upload_partpath_;   // 临时文件路径（.part）
    std::string upload_finalpath_;  // 最终文件路径

    // multipart 专用
    std::string mp_boundary_;   // boundary 原值
    std::string mp_endmarker_;  // 结尾标记 "\r\n--boundary--"
    std::string mp_preamble_;   // part 头累积（很小）
    std::string mp_tail_;       // 为跨 chunk 检测 marker 而保留的尾部
    bool mp_hdr_done_;          // part 头是否已解析、文件是否已打开
};