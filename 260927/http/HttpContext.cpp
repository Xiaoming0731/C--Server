#include "HttpContext.h"
#include "HttpRequest.h"
#include <memory>
#include <string>
#include <algorithm>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <cerrno>
#include <cctype>

// 简单 URL 解码（处理 %XX 和 +）
static std::string UrlDecode(const std::string &s){
    std::string out;
    out.reserve(s.size());
    auto hex = [](char c)->int{
        if(c >= '0' && c <= '9') return c - '0';
        if(c >= 'a' && c <= 'f') return c - 'a' + 10;
        return c - 'A' + 10;
    };
    for(size_t i = 0; i < s.size(); ++i){
        if(s[i] == '%' && i + 2 < s.size() &&
           isxdigit((unsigned char)s[i+1]) && isxdigit((unsigned char)s[i+2])){
            out.push_back((char)(hex(s[i+1]) * 16 + hex(s[i+2])));
            i += 2;
        }else if(s[i] == '+'){
            out.push_back(' ');
        }else{
            out.push_back(s[i]);
        }
    }
    return out;
}

HttpContext::HttpContext()
    : state_(HttpRequestParaseState::START),
      parsed_len_(0),
      upload_fd_(-1),
      upload_remaining_(0),
      mp_hdr_done_(false) {
    request_ = std::make_unique<HttpRequest>();
}
HttpContext::~HttpContext(){
    if(upload_fd_ != -1) ::close(upload_fd_);
};


bool HttpContext::GetCompleteRequest(){
    return state_ == HttpRequestParaseState::COMPLETE;
}

void HttpContext::ResetContextStatus(){
   state_ = HttpRequestParaseState::START;
   parsed_len_ = 0;
   request_ = std::make_unique<HttpRequest>();
}

bool HttpContext::ParaseRequest(Buffer *buffer){
    // 流式上传进行中：直接把新到的 body 写入文件
    if (state_ == HttpRequestParaseState::STREAM_UPLOAD) {
        if (StreamUploadToFile(buffer)) {
            state_ = HttpRequestParaseState::COMPLETE;
            return true;
        }
        return false; // 还没到齐，等下次读事件
    }

    // multipart 流式上传进行中
    if (state_ == HttpRequestParaseState::MP_UPLOAD) {
        if (ProcessMultipart(buffer)) {
            state_ = HttpRequestParaseState::COMPLETE;
            return true;
        }
        return false;
    }

    int readable = buffer->readablebytes();
    if (readable <= 0) return false;

    const char *begin = buffer->Peek();
    const char *end = begin + readable;
    const char *start = begin + parsed_len_;  // 从上次断点继续
    const char *colon = start;
    const char *p = start;

    while(state_ != HttpRequestParaseState::kINVALID
        && state_ != HttpRequestParaseState::COMPLETE
        && p < end) {
        
        char ch = *p;
        switch (state_){
            case HttpRequestParaseState::START:{ 
                if(ch == CR || ch== LF || isblank(ch)){
                    ++p;
                }else if(isupper(ch)){
                    state_ = HttpRequestParaseState::METHOD;
                    start = p;
                    ++p;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::METHOD:{
                if(isupper(ch)){
                    ++p;
                }else if(isblank(ch)){
                    request_->SetMethod(std::string(start, p));
                    state_ = HttpRequestParaseState::BEFORE_URL;
                    ++p;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_URL:{
                if(ch == '/'){
                    state_ = HttpRequestParaseState::IN_URL;
                    start = p;
                    ++p;
                } else if (isblank(ch)){
                    ++p;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::IN_URL:{
                if(ch == '?'){
                    request_->SetUrl(std::string(start, p));
                    start = p + 1;
                    state_ = HttpRequestParaseState::BEFORE_URL_PARAM_KEY;
                    ++p;
                }else if (isblank(ch)){
                    request_->SetUrl(std::string(start, p));
                    state_ = HttpRequestParaseState::BEFORE_PROTOCOL;
                    ++p;
                }else{
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_URL_PARAM_KEY:{
                if(isblank(ch) || ch == CR || ch == LF){
                    state_ = HttpRequestParaseState::kINVALID;
                }else{
                    state_ = HttpRequestParaseState::URL_PARAM_KEY;
                    start = p;
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::URL_PARAM_KEY:{
                if(ch == '='){
                    colon = p;
                    state_ = HttpRequestParaseState::BEFORE_URL_PARAM_VALUE;
                    ++p;
                }else if(isblank(ch)){
                    state_ = HttpRequestParaseState::kINVALID;
                }else{
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_URL_PARAM_VALUE:{
                if(isblank(ch) || ch == LF || ch == CR){
                    state_ = HttpRequestParaseState::kINVALID;
                }else{
                    state_ = HttpRequestParaseState::URL_PARAM_VALUE;
                    // 不能再 start = p：start 必须保留“键的开头”，
                    // 值的开头用 colon + 1 表示，否则 string(start,colon) 指针倒置
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::URL_PARAM_VALUE:{
                if(ch == '&'){
                    request_->SetRequestParams(std::string(start, colon), std::string(colon + 1, p));
                    state_ = HttpRequestParaseState::BEFORE_URL_PARAM_KEY;
                    ++p;
                }
                if(isblank(ch)){
                    request_->SetRequestParams(std::string(start, colon), std::string(colon + 1, p));
                    state_ = HttpRequestParaseState::BEFORE_PROTOCOL;
                    ++p;
                }else{
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_PROTOCOL:{
                if(isblank(ch)){
                    ++p;
                }else{
                    state_ = HttpRequestParaseState::PROTOCOL;
                    start = p;
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::PROTOCOL:{
                if(ch == '/'){
                    request_->SetProtocol(std::string(start, p));
                    state_ = HttpRequestParaseState::BEFORE_VERSION;
                    ++p;
                }else{
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_VERSION:{
                if(isdigit(ch)){
                    state_ = HttpRequestParaseState::VERSION;
                    start = p;
                    ++p;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::VERSION:{
                if(ch == CR){
                    request_->SetVersion(std::string(start, p));
                    state_ = HttpRequestParaseState::WHEN_CR;
                    ++p;
                }else if(isdigit(ch) || ch == '.'){
                    ++p;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::HEADER_KEY:{
                if(ch == ':'){
                    colon = p;
                    state_ = HttpRequestParaseState::HEADER_VALUE;
                    ++p;
                }else{
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::HEADER_VALUE:{
                if(isblank(ch)){
                    ++p;
                }else if(ch == CR){
                    request_->AddHeader(std::string(start, colon), std::string(colon + 2, p));
                    state_ = HttpRequestParaseState::WHEN_CR;
                    ++p;
                }else{
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::WHEN_CR:{
                if(ch == LF){
                    state_ = HttpRequestParaseState::CR_LF;
                    ++p;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::CR_LF:{
                if(ch == CR){
                    state_ = HttpRequestParaseState::CR_LF_CR;
                    ++p;
                }else if(isblank(ch)){
                    state_ = HttpRequestParaseState::kINVALID;
                }else{
                    state_ = HttpRequestParaseState::HEADER_KEY;
                    start = p;
                    ++p;
                }
                break;
            }
            case HttpRequestParaseState::CR_LF_CR:{
                if(ch == LF){
                    ++p;
                    if(request_->headers().count("Content-Length")){
                        // multipart 流式上传：/upload
                        if(request_->url() == "/upload" &&
                           request_->GetHeader("Content-Type").find("multipart/form-data") != std::string::npos){
                            // 消费掉请求头，让 buffer 从 body 开头
                            buffer->Retrieve(static_cast<int>(p - buffer->Peek()));
                            if(!BeginMultipartUpload()){
                                state_ = HttpRequestParaseState::kINVALID;
                                break;
                            }
                            state_ = HttpRequestParaseState::MP_UPLOAD;
                            if(ProcessMultipart(buffer)){
                                state_ = HttpRequestParaseState::COMPLETE;
                                return true;
                            }
                            return false;
                        }
                        // 流式原始上传：/upload_raw?filename=xxx，body 即文件内容
                        if(request_->url() == "/upload_raw"){
                            // 先把请求头（p 之前的字节）从 buffer 消费掉，
                            // 这样 StreamUploadToFile 从 body 开头写，不会把头写进文件
                            buffer->Retrieve(static_cast<int>(p - buffer->Peek()));
                            if(!BeginStreamUpload()){
                                state_ = HttpRequestParaseState::kINVALID;
                                break;
                            }
                            state_ = HttpRequestParaseState::STREAM_UPLOAD;
                            if(StreamUploadToFile(buffer)){
                                state_ = HttpRequestParaseState::COMPLETE;
                                return true;
                            }
                            return false; // 等剩余 body
                        }
                        long len = atol(request_->GetHeader("Content-Length").c_str());
                        int body_bytes = static_cast<int>(end - p);
                        if(body_bytes >= len){
                            request_->SetBody(std::string(p, p + len));
                            p += len;
                            state_ = HttpRequestParaseState::COMPLETE;
                        }else{
                            // body 没到齐：切到 BODY 状态等下次数据，p 留在 body 开头
                            state_ = HttpRequestParaseState::BODY;
                        }
                    }else{
                        state_ = HttpRequestParaseState::COMPLETE;
                    }
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::BODY:{
                long len = atol(request_->GetHeader("Content-Length").c_str());
                int body_bytes = static_cast<int>(end - p);  // p 始终在 body 开头
                if(body_bytes >= len){
                    request_->SetBody(std::string(p, p + len));
                    p += len;
                    state_ = HttpRequestParaseState::COMPLETE;
                }
                // 不够：p 不动，结束本次解析，等下次读事件
                break;
            }
            default:
                state_ = HttpRequestParaseState::kINVALID;
                break;
        }

        // body 还没到齐：结束本次解析，等下次读事件（p 保持在 body 开头）
        if (state_ == HttpRequestParaseState::BODY) {
            break;
        }
    }

    // 更新已解析长度
    parsed_len_ = static_cast<int>(p - begin);

    // 解析完成或出错：只消费掉已解析的字节
    if (state_ == HttpRequestParaseState::COMPLETE || state_ == HttpRequestParaseState::kINVALID) {
        buffer->Retrieve(parsed_len_);
    }

    return state_ == HttpRequestParaseState::COMPLETE ||
       state_ == HttpRequestParaseState::BODY;
}


// 打开临时文件，准备流式写入
bool HttpContext::BeginStreamUpload(){
    std::string raw = request_->GetRequestValue("filename");
    std::string filename = UrlDecode(raw);

    // 防路径穿越：只保留最后一段
    size_t slash = filename.find_last_of('/');
    if(slash != std::string::npos) filename = filename.substr(slash + 1);
    size_t bslash = filename.find_last_of('\\');
    if(bslash != std::string::npos) filename = filename.substr(bslash + 1);
    if(filename.empty() || filename == "." || filename == "..") return false;

    upload_partpath_  = "../files/" + filename + ".part";
    upload_finalpath_ = "../files/" + filename;

    upload_fd_ = ::open(upload_partpath_.c_str(),
                        O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(upload_fd_ == -1) return false;

    upload_remaining_ = atoll(request_->GetHeader("Content-Length").c_str());
    return true;
}

// 把 buffer 中现有的 body 字节写入文件（写完即从 buffer 消费，内存不累积）
// 全部到齐则关闭、原子改名，返回 true
bool HttpContext::StreamUploadToFile(Buffer *buffer){
    while(true){
        int readable = buffer->readablebytes();
        long long n = std::min<long long>(readable, upload_remaining_);

        if(n > 0){
            const char *data = buffer->Peek();
            long long off = 0;
            while(off < n){
                ssize_t w = ::write(upload_fd_, data + off, (size_t)(n - off));
                if(w > 0){
                    off += w;
                }else if(w < 0 && errno == EINTR){
                    continue;
                }else{
                    return false; // 写盘错误
                }
            }
            buffer->Retrieve(static_cast<int>(n));
            upload_remaining_ -= n;
        }

        if(upload_remaining_ == 0){
            ::close(upload_fd_);
            upload_fd_ = -1;
            // 原子提交：临时文件改名为最终文件
            if(std::rename(upload_partpath_.c_str(), upload_finalpath_.c_str()) != 0){
                return false;
            }
            return true;
        }

        if(buffer->readablebytes() == 0) return false; // 等下次读事件
    }
}


// ============ multipart 流式上传 ============

bool HttpContext::BeginMultipartUpload(){
    std::string ct = request_->GetHeader("Content-Type");
    size_t bpos = ct.find("boundary=");
    if(bpos == std::string::npos) return false;
    mp_boundary_ = ct.substr(bpos + 9);
    // 浏览器在 body 里的结尾标记：\r\n--boundary--
    mp_endmarker_ = "\r\n--" + mp_boundary_ + "--";
    mp_preamble_.clear();
    mp_tail_.clear();
    mp_hdr_done_ = false;
    return true;
}

bool HttpContext::MpWriteToFile(const char *data, size_t len){
    size_t off = 0;
    while(off < len){
        ssize_t w = ::write(upload_fd_, data + off, len - off);
        if(w > 0){
            off += (size_t)w;
        }else if(w < 0 && errno == EINTR){
            continue;
        }else{
            return false;
        }
    }
    return true;
}

bool HttpContext::MpFinalize(){
    if(upload_fd_ != -1){
        ::close(upload_fd_);
        upload_fd_ = -1;
    }
    return std::rename(upload_partpath_.c_str(), upload_finalpath_.c_str()) == 0;
}

bool HttpContext::ProcessMultipart(Buffer *buffer){
    int readable = buffer->readablebytes();
    if(readable <= 0) return false;

    // 先把本次新到数据取出，只从 buffer 消费一次
    std::string chunk(buffer->Peek(), readable);
    buffer->Retrieve(readable);

    if(!mp_hdr_done_){
        // part 头很小，先累积到空行
        mp_preamble_ += chunk;

        size_t sep = mp_preamble_.find("\r\n\r\n");
        if(sep == std::string::npos){
            return false; // part 头还没到齐
        }

        // 解析 filename
        size_t fn_pos = mp_preamble_.find("filename=\"");
        if(fn_pos == std::string::npos) return false;
        fn_pos += 10;
        size_t fn_end = mp_preamble_.find("\"\r\n", fn_pos);
        if(fn_end == std::string::npos) return false;
        std::string filename = mp_preamble_.substr(fn_pos, fn_end - fn_pos);

        // 防路径穿越
        size_t sl = filename.find_last_of('/');
        if(sl != std::string::npos) filename = filename.substr(sl + 1);
        size_t bs = filename.find_last_of('\\');
        if(bs != std::string::npos) filename = filename.substr(bs + 1);
        if(filename.empty()) return false;

        upload_partpath_  = "../files/" + filename + ".part";
        upload_finalpath_ = "../files/" + filename;
        upload_fd_ = ::open(upload_partpath_.c_str(),
                           O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(upload_fd_ == -1) return false;
        mp_hdr_done_ = true;

        // 空行之后的字节即文件内容开头（已包含本次 chunk），放入 mp_tail
        mp_tail_ = mp_preamble_.substr(sep + 4);
        mp_preamble_.clear();
    }else{
        // part 头已处理：新数据并入待检测缓冲
        mp_tail_ += chunk;
    }

    // 找结尾标记
    size_t found = mp_tail_.find(mp_endmarker_);
    if(found != std::string::npos){
        // found 之前即文件内容（mp_endmarker_ 以 \r\n 开头，已被排除）
        if(found > 0 && !MpWriteToFile(mp_tail_.data(), found)) return false;
        mp_tail_.clear();
        return MpFinalize();
    }

    // 没找到：保留末尾 (marker长度-1) 字节，防止 marker 跨 chunk
    size_t hold = mp_endmarker_.size() - 1;
    if(mp_tail_.size() > hold){
        size_t safe = mp_tail_.size() - hold;
        if(!MpWriteToFile(mp_tail_.data(), safe)) return false;
        mp_tail_ = mp_tail_.substr(safe);
    }
    return false;
}


bool HttpContext::ParaseRequest(const char *begin, int size){

    state_ = HttpRequestParaseState::START;
    request_ = std::make_unique<HttpRequest>();

    const char *start = begin;   // 去掉const_cast，统一用const char*
    const char *end = start;
    const char *colon = end;
    while(state_ != HttpRequestParaseState::kINVALID
        && state_ != HttpRequestParaseState::COMPLETE
        && end - begin < size){          // 原来是 <= size，会越界
        char ch = *end;
        //std::cout << ch;
        switch (state_){
            case HttpRequestParaseState::START:{ 
                if(ch == CR || ch== LF || isblank(ch)){
                    // 遇到空格，换行和回车都继续。
                }else if(isupper(ch)){
                    // 遇到大写字母，说明遇到了METHOD
                    state_ = HttpRequestParaseState::METHOD;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::METHOD:{
                if(isupper(ch)){
                    // 如果是大写字母，则继续
                }else if(isblank(ch)){
                    // 遇到空格表明，METHOD方法解析结束，当前处于即将解析URL，start进入下一个位置
                    request_->SetMethod(std::string(start, end));
                    state_ = HttpRequestParaseState::BEFORE_URL;
                    start = end + 1; // 更新下一个指标的位置
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_URL:{
                // 对请求连接前的处理，请求连接以'/'开头
                if(ch == '/'){
                    // 遇到/ 说明遇到了URL，开始解析
                    state_ = HttpRequestParaseState::IN_URL;
                } else if (isblank(ch)){

                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::IN_URL:{
                // 进入url中
                if(ch == '?'){
                    // 当遇到?时，表明进入了request params的处理。
                    request_->SetUrl(std::string(start, end));
                    start = end + 1;
                    state_ = HttpRequestParaseState::BEFORE_URL_PARAM_KEY;
                   
                    
                }else if (isblank(ch)){
                    // 说明没有请求参数，请求路径完成
                    request_->SetUrl(std::string(start, end));
                    start = end + 1;
                    state_ = HttpRequestParaseState::BEFORE_PROTOCOL;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_URL_PARAM_KEY:{
                //std::cout << ch << std::endl;
                if(isblank(ch) || ch == CR || ch == LF){
                    // 当开始进入url params时，遇到了空格，换行等，则不合法
                    // std::cout << ch << std::endl;
                    state_ = HttpRequestParaseState::kINVALID;
                }else{
                    state_ = HttpRequestParaseState::URL_PARAM_KEY;
                }
                break;
            }
            case HttpRequestParaseState::URL_PARAM_KEY:{
                if(ch == '='){
                    // 遇到= 说明key解析完成
                    colon = end;
                    state_ = HttpRequestParaseState::BEFORE_URL_PARAM_VALUE;
                }else if(isblank(ch)){
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::BEFORE_URL_PARAM_VALUE:{
                if(isblank(ch) || ch == LF || ch == CR){
                    state_ = HttpRequestParaseState::kINVALID;
                }else{
                    state_ = HttpRequestParaseState::URL_PARAM_VALUE;
                }
                break;
            }

            case HttpRequestParaseState::URL_PARAM_VALUE:{
                if(ch == '&'){
                    // 说明遇到了下一个请求参数
                    state_ = HttpRequestParaseState::BEFORE_URL_PARAM_KEY;
                    request_->SetRequestParams(std::string(start, colon), std::string(colon + 1, end));
                    start = end + 1;
                }
                if(isblank(ch)){
                    // 遇到空格，说明解析结束。
                    request_->SetRequestParams(std::string(start, colon), std::string(colon + 1, end));
                    start = end + 1;
                    state_ = HttpRequestParaseState::BEFORE_PROTOCOL;
                }else{

                }
                break;
            }
            case HttpRequestParaseState::BEFORE_PROTOCOL:{
                //std::cout << std::string(start, end) << std::endl;
                if(isblank(ch)){
                    // nothing
                }else{
                    state_ = HttpRequestParaseState::PROTOCOL;
                }
                break;
            }
            case HttpRequestParaseState::PROTOCOL:{
                if(ch == '/'){
                    request_->SetProtocol(std::string(start, end));
                    start = end + 1;
                    state_ = HttpRequestParaseState::BEFORE_VERSION;
                }else{

                }
                break;
            }

            case HttpRequestParaseState::BEFORE_VERSION:{
                if(isdigit(ch)){
                    state_ = HttpRequestParaseState::VERSION;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }

            case HttpRequestParaseState::VERSION:{
                if(ch == CR){
                    // 说明结束了
                    request_->SetVersion(std::string(start, end));
                    start = end + 1;
                    state_ = HttpRequestParaseState::WHEN_CR;
                }else if(isdigit(ch) || ch == '.'){
                    
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }

            // 需要注意的是，对header的解析并不鲁棒
            case HttpRequestParaseState::HEADER_KEY:{
                if(ch == ':'){
                    colon = end;
                    state_ = HttpRequestParaseState::HEADER_VALUE;
                }
                break;
            }
            case HttpRequestParaseState::HEADER_VALUE:{
                if(isblank(ch)){

                }else if(ch == CR){
                    request_->AddHeader(std::string(start, colon), std::string(colon + 2, end));
                    start = end + 1;
                    state_ = HttpRequestParaseState::WHEN_CR;
                }
                break;
            }

            case HttpRequestParaseState::WHEN_CR:{
                if(ch == LF){
                    // 如果遇到了'\n'之后遇到了'\r'，那就意味着这一行结束了
                    start = end + 1;
                    state_ = HttpRequestParaseState::CR_LF;
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::CR_LF:{
                //std::cout << "111" << ch << std::endl;
                if(ch == CR){
                    // 说明遇到了空行，大概率时结束了
                    state_ = HttpRequestParaseState::CR_LF_CR;
                    //start  = end + 1;
                    //std::cout << "a:" << (*start == '\n') << std::endl;
                    //std::cout << "b:" << (*end == '\r') << std::endl;
                }else if(isblank(ch)){
                    state_ = HttpRequestParaseState::kINVALID;
                }else{
                    state_ = HttpRequestParaseState::HEADER_KEY;
                }
                break;
            }
            case HttpRequestParaseState::CR_LF_CR:{
                if(ch == LF){
                    start = end + 1;   // start 指向 body 开头（本函数用 start 做标记、end 做移动指针）
                    if(request_->headers().count("Content-Length")){
                        long len = atol(request_->GetHeader("Content-Length").c_str());
                        int body_bytes = static_cast<int>((begin + size) - start);
                        if(body_bytes >= len){
                            request_->SetBody(std::string(start, start + len));
                            state_ = HttpRequestParaseState::COMPLETE;
                        }else{
                            // body 没到齐：切到 BODY 状态等下次数据
                            state_ = HttpRequestParaseState::BODY;
                        }
                    }else{
                        state_ = HttpRequestParaseState::COMPLETE;
                    }
                }else{
                    state_ = HttpRequestParaseState::kINVALID;
                }
                break;
            }
            case HttpRequestParaseState::BODY:{
                long len = atol(request_->GetHeader("Content-Length").c_str());
                int body_bytes = static_cast<int>((begin + size) - start);  // start 始终在 body 开头
                if(body_bytes >= len){
                    request_->SetBody(std::string(start, start + len));
                    state_ = HttpRequestParaseState::COMPLETE;
                }
                // 不够：结束本次解析，等下次数据
                break;
            }

            default:
                state_ = HttpRequestParaseState::kINVALID;
                break;
            }
            if (state_ == HttpRequestParaseState::BODY) {
                break;
            }
            
            
           end++;
    }
    return state_ == HttpRequestParaseState::COMPLETE;
}

HttpRequest * HttpContext::request(){
    return request_.get();
}

bool HttpContext::HasError() const {
    return state_ == HttpRequestParaseState::kINVALID;
}