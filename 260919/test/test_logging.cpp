#include "Logging.h"

int main(){
    // 1. 不同级别输出
    LOG_DEBUG << "debug message";
    LOG_INFO  << "info message";
    LOG_WARN  << "warn message";
    LOG_ERROR << "error message";

    // 2. 链式输出 + 数字类型
    LOG_INFO << "fd=" << 56 << " port=" << 8080 << " ratio=" << 3.14 ;

    // 3. 测试级别过滤：设置全局级别为 WARN 后，DEBUG/INFO 不再输出
    Logger::setLogLevel(Logger::WARN);
    LOG_DEBUG << "should NOT appear";
    LOG_INFO  << "should NOT appear";
    LOG_WARN  << "warn still appears";
    return 0;
}