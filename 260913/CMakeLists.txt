# cmake_minimum_required(VERSION 3.10)
# project(echo_server)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # 编译参数：调试符号、开启警告
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
# set(CMAKE_CXX_FLAGS_DEBUG "-O0")

# # 头文件搜索路径，base、tcp两个目录
# include_directories(base tcp)

# # 搜集base目录所有cpp
# aux_source_directory(base BASE_SRC)
# # 搜集tcp目录所有cpp
# aux_source_directory(tcp TCP_SRC)

# # 查找pthread线程库
# find_package(Threads REQUIRED)

# # 输出可执行文件到 build/bin
# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

# # echo_server.cpp 在项目【根目录】，不是在example！
# add_executable(echoserver
#     echo_server.cpp
#     ${BASE_SRC}
#     ${TCP_SRC}
# )

# target_link_libraries(echoserver Threads::Threads)



CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
project(echo_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")

include_directories(tcp base)
aux_source_directory(tcp  TCP_SRC)
aux_source_directory(base BASE_SRC)

find_package(Threads REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

add_executable(echoserver
    echo_server.cpp
    ${BASE_SRC}
    ${TCP_SRC}
)


target_link_libraries(echoserver Threads::Threads)