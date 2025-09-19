#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool {
public:
    // 构造函数，创建指定数量的线程
    explicit ThreadPool(size_t numThreads);
    // 析构函数，停止所有线程
    ~ThreadPool();
    // 添加任务到线程池
    template<class F, class... Args>
    void addTask(F&& f, Args&&... args);
private:
    // 工作线程函数
    void worker();
private:
    // 工作线程容器
    std::vector<std::thread> m_workers;
    // 任务队列
    std::queue<std::function<void()>> m_tasks;
    // 互斥锁，用于保护任务队列
    std::mutex m_queueMutex;
    // 条件变量，用于通知工作线程有新任务
    std::condition_variable m_condition;
    // 原子变量，表示线程池是否停止
    std::atomic<bool> m_stop;
};

// 模板函数实现
template<class F, class... Args>
void ThreadPool::addTask(F&& f, Args&&... args) {
    // 将任务和参数包装成一个无参数的函数
    auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        
        // 如果线程池已经停止，不允许添加新任务
        if (m_stop) {
            throw std::runtime_error("addTask on stopped ThreadPool");
        }
        
        // 将任务添加到队列
        m_tasks.emplace([task]() { task(); });
    }
    
    // 通知一个工作线程有新任务
    m_condition.notify_one();
}