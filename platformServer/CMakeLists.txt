cmake_minimum_required(VERSION 3.15)
project(Server)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找必要的包
# OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "Found OpenSSL: ${OPENSSL_INCLUDE_DIR}")

# JSONCPP（使用pkg_check_modules，无需手动设置路径）
# 方法1：对于通过包管理器安装的 JSONCPP（推荐）
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
if(JSONCPP_FOUND)
    message(STATUS "找到 JSONCPP: ${JSONCPP_VERSION}")
    message(STATUS "JSONCPP 头文件路径: ${JSONCPP_INCLUDE_DIRS}")
    message(STATUS "JSONCPP 库路径: ${JSONCPP_LIBRARIES}")
endif()

# Protobuf
find_package(Protobuf REQUIRED)
if(PROTOBUF_FOUND)
    message(STATUS "找到Protobuf: ${Protobuf_VERSION}")
    message(STATUS "Protobuf头文件路径: ${PROTOBUF_INCLUDE_DIRS}")
    message(STATUS "Protobuf库路径: ${PROTOBUF_LIBRARIES}")
else()
    message(FATAL_ERROR "未找到Protobuf，请检查安装")
endif()

# MySQL
set(MYSQL_INCLUDE_DIR "/usr/include/mysql")
set(MYSQL_LIBRARY "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")
message(STATUS "Found MySQL: ${MYSQL_INCLUDE_DIR}")

# MySQL Connector/C++
set(MYSQLCPPCONN_INCLUDE_DIR "/usr/include/mysql-cppconn")
set(MYSQLCPPCONN_LIBRARY "/usr/lib/x86_64-linux-gnu/libmysqlcppconn.so")
message(STATUS "Found MySQL Connector/C++: ${MYSQLCPPCONN_INCLUDE_DIR}")

# 1. 收集源代码目录下的所有.cpp
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/server/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/interface/*.cpp"

)
# 显式追加 Protobuf 生成的 .cc 文件
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/server/message.pb.cc")
# 2. 手动添加提前生成的.pb.cc文件（替换为你的实际路径）
# 假设.pb.cc放在proto_gen/目录下
file(GLOB PREBUILT_PROTO_SRCS 
    "${CMAKE_CURRENT_SOURCE_DIR}/server/*.pb.cc"  # 你的.pb.cc所在目录
)
# 调试用：确认.pb.cc被包含
message(STATUS "包含的Protobuf源文件: ${PREBUILT_PROTO_SRCS}")
message(STATUS "所有源文件列表: ${SOURCES}")
# 添加可执行文件
add_executable(server ${SOURCES})

# 添加头文件路径（包含提前生成的.pb.h所在目录）
target_include_directories(server PRIVATE
    ${OPENSSL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/server 
    ${MYSQL_INCLUDE_DIR}
    ${MYSQLCPPCONN_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
)

# 链接库
target_link_libraries(server
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
    mysqlclient
    mysqlcppconn
    mysqlcppconnx
    protobuf  # 链接Protobuf库
    pthread
)

# 编译选项
target_compile_options(server PRIVATE -Wno-deprecated-declarations)

# 调试信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "可执行文件输出路径: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
