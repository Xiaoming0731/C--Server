#include "HttpContext.h"
#include "HttpRequest.h"
#include <memory>
#include <string>
#include <algorithm>
#include <iostream>

HttpContext::HttpContext() 
    : state_(HttpRequestParaseState::START),
      parsed_len_(0) {
    request_ = std::make_unique<HttpRequest>();
}
HttpContext::~HttpContext(){
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
                    start = p;
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