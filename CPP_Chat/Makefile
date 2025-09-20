# CPP_Chat项目Makefile

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -I./src/common

# 目标文件
CLIENT_WIN_TARGET = build/client/client.exe
CLIENT_LINUX_TARGET = build/client/client
SERVER_TARGET = build/server/server

# 源文件
COMMON_SRCS = $(wildcard src/common/*.cpp)
CLIENT_SRCS = src/client/clientmain.cpp $(wildcard src/client/*.cpp)
SERVER_SRCS = src/server/main.cpp $(wildcard src/server/*.cpp)

# 目标文件
COMMON_OBJS = $(COMMON_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o) $(COMMON_OBJS)
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o) $(COMMON_OBJS)

# 默认目标
.PHONY: all clean client_win client_linux server

all: client_win client_linux server

# Windows客户端目标
client_win: $(CLIENT_OBJS)
	@mkdir -p build/client
	$(CXX) $(CXXFLAGS) -o $(CLIENT_WIN_TARGET) $^ -lws2_32 -ljsoncpp

# Linux客户端目标
client: $(CLIENT_OBJS)
	@mkdir -p build/client
	$(CXX) $(CXXFLAGS) -o $(CLIENT_LINUX_TARGET) $^ -ljsoncpp

# 服务器端目标（Linux）
server: $(SERVER_OBJS)
	@mkdir -p build/server
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $^ -levent -lmysqlclient -lpthread -ljsoncpp

# 编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) main.o $(CLIENT_WIN_TARGET) $(CLIENT_LINUX_TARGET) $(SERVER_TARGET)