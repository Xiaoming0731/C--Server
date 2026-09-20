
cmake_minimum_required(VERSION 3.10)
project(WebServer)


include_directories(
    base
    tcp
    log
    http
    timer
)

SET(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++14")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

aux_source_directory(base SRC_LIST1)
aux_source_directory(tcp SRC_LIST2)
aux_source_directory(http SRC_LIST3)
aux_source_directory(timer SRC_LIST4)
aux_source_directory(log SRC_LIST5)


set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/test/)

# echoserver 补上 http 目录源码（因为 TcpConnection 现在依赖 HttpContext）
# add_executable(echoserver echo_server.cpp ${SRC_LIST1} ${SRC_LIST2} ${SRC_LIST3})
# # test_context 补上 base 和 tcp 目录源码（因为 HttpServer 依赖底层网络库）
# add_executable(test_context test/test_httpcontext.cpp ${SRC_LIST1} ${SRC_LIST2} ${SRC_LIST3})
# add_executable(http_server test/http_server.cpp ${SRC_LIST1} ${SRC_LIST2} ${SRC_LIST3})

# target_link_libraries(echoserver ${CMAKE_THREAD_LIBS_INIT})

add_executable(webserver test/http_server.cpp ${SRC_LIST1} ${SRC_LIST2} ${SRC_LIST3} ${SRC_LIST4} ${SRC_LIST5})
add_executable(test_logger test/test_logging.cpp ${SRC_LIST1} ${SRC_LIST5})

# 改后
add_executable(test_logstream test/test_logstream.cpp ${SRC_LIST5} ${SRC_LIST1})
target_link_libraries(webserver ${CMAKE_THREAD_LIBS_INIT})