# PlatformClient和PlatformServer密钥管理系统

## 简介

PlatformClient和PlatformServer是一个密钥管理系统，用于安全地管理客户端和服务器之间的密钥协商、校验和注销。系统采用RSA非对称加密和AES对称加密技术，确保密钥传输和存储的安全性。

## 系统要求

- 操作系统：Ubuntu 22.04
- C++编译器（支持C++11或更高版本）
- CMake构建工具
- 依赖库：
  - jsoncpp（用于JSON数据处理）
  - mysqlx（MySQL C++连接器）
  - protobuf（Protocol Buffers，用于数据序列化）
  - openssl（用于加密操作）

## 安装说明

### 1. 安装依赖库

```bash
# 在Ubuntu 22.04上安装依赖库
sudo apt update
sudo apt install cmake build-essential libjsoncpp-dev libmysql-connector-cpp-dev libprotobuf-dev protobuf-compiler libssl-dev
```

### 2. 数据库准备

```bash
mysql -u root -p < mysql.sql
```

### 3. 编译客户端

```bash
cd platformClient
mkdir build && cd build
cmake ..
make
```

### 4. 编译服务器

```bash
cd platformServer
mkdir build && cd build
cmake ..
make
```

## 使用方法

### 1. 启动服务器

```bash
cd platformServer/build/bin
./server
```

### 2. 启动客户端

```bash
cd platformClient/build/bin
./client
```

### 3. 客户端功能

- 1. 密钥协商
- 2. 密钥校验
- 3. 密钥注销
- 0. 退出系统

## 配置文件

- 客户端配置：`platformClient/client.json`
- 服务器配置：`platformServer/server.json`

## 文档

详细使用说明请参考`PlatformClient和PlatformServer使用文档.md`。