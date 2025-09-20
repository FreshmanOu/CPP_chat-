#include "server.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    cpp_chat::Server server("192.168.149.129", 8888, 2);
    
    std::cout << "Starting server..." << std::endl;
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    // 保持服务器运行
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}