# CPP_Chat 即时通讯项目

这是一个跨平台的C++即时通讯项目，包含客户端（Windows/Linux）和服务器端（Linux）。

## 项目结构

```
CPP_Chat/
├── src/
│   ├── client/       # 客户端代码
│   ├── common/       # 公共代码
│   └── server/       # 服务器端代码
├── docs/            # 文档
├── tests/           # 测试
├── build/           # 构建输出目录
├── Makefile         # 通用Makefile
├── build_client.bat # Windows客户端构建脚本
├── build_client_linux.sh # Linux客户端构建脚本
├── build_server.sh  # Linux服务器端构建脚本
├── clean.bat        # Windows清理脚本
└── clean.sh         # Linux清理脚本
```

## 构建说明

### Windows客户端构建

在Windows环境下，可以使用以下方法构建客户端：

#### 使用批处理文件

1. 确保已安装MinGW或其他G++编译器
2. 双击运行`build_client.bat`或在命令行中执行：

```cmd
build_client.bat
```

#### 手动编译

```cmd
mkdir -p build\client
g++ -std=c++11 -I.\src\common src\client\*.cpp src\common\*.cpp -o build\client\cpp_chat_client.exe -lws2_32
```

### Linux客户端构建

在Linux环境下，可以使用以下方法构建客户端：

#### 使用Makefile

```bash
make client_linux
```

#### 手动编译

```bash
mkdir -p build/client
g++ -std=c++11 -I./src/common src/client/*.cpp src/common/*.cpp -o build/client/cpp_chat_client
```

### Linux服务器端构建

在Linux环境下，可以使用以下方法构建服务器端：


#### 使用Makefile

```bash
make server
```

#### 手动编译

```bash
mkdir -p build/server
g++ -std=c++11 -I./src/common src/server/*.cpp src/common/*.cpp -o build/server/cpp_chat_server -levent -lmysqlclient -lpthread
```

## 依赖库

### 客户端依赖
- WinSock2 (仅Windows客户端需要，Windows系统自带)
- 标准网络库 (Linux客户端)

### 服务器端依赖
- libevent
- MySQL客户端库
- pthread

#### 安装依赖（Ubuntu/Debian）
```bash
sudo apt-get install libevent-dev libmysqlclient-dev
```

#### 安装依赖（CentOS/RHEL）
```bash
sudo yum install libevent-devel mysql-devel
```

## 运行

### 运行服务器端
```bash
./build/server/server
```

### 运行客户端

#### Windows
```cmd
.\build\client\client.exe
```

#### Linux
```bash
./build/client/client
```