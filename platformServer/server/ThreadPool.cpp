#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t numThreads) : m_stop(false) {
    // 创建指定数量的工作线程
    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back([this] {
            worker();
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    
    // 通知所有工作线程
    m_condition.notify_all();
    
    // 等待所有工作线程结束
    for (std::thread& worker : m_workers) {
        worker.join();
    }
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            // 等待条件变量：任务队列不为空或线程池停止
            m_condition.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });
            
            // 如果线程池停止且任务队列为空，退出线程
            if (m_stop && m_tasks.empty()) {
                return;
            }
            
            // 从任务队列取出一个任务
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        
        // 执行任务
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in thread pool task" << std::endl;
        }
    }
}