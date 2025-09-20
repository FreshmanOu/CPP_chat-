@echo off
echo 编译CPP_Chat客户端...

:: 创建构建目录
if not exist build\client mkdir build\client

:: 设置编译器和选项
set CXX=g++
set CXXFLAGS=-std=c++11 -Wall -I./src/common

:: 编译common文件
echo 编译common文件...
for %%f in (src\common\*.cpp) do (
    echo 编译 %%f
    %CXX% %CXXFLAGS% -c %%f -o %%~nf.o
)

:: 编译client文件
echo 编译client文件...
%CXX% %CXXFLAGS% -c src\client\main.cpp -o main.o
for %%f in (src\client\*.cpp) do (
    echo 编译 %%f
    %CXX% %CXXFLAGS% -c %%f -o %%~nf.o
)

:: 链接
echo 链接...
%CXX% %CXXFLAGS% -o build\client\cpp_chat_client.exe *.o -lws2_32

:: 清理.o文件
echo 清理临时文件...
del *.o main.o

echo 编译完成！
echo 客户端可执行文件位于: build\client\cpp_chat_client.exe