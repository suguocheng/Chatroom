#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t num_threads) : stop(false) {
    // 使用 std::thread::hardware_concurrency() 获取系统的硬件并发数
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
    }

    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([this] {
            worker_thread();
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all();
    for (std::thread &worker : threads) {
        worker.join();
    }
}

template<class F>
void ThreadPool::add_task(F&& f) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace(std::forward<F>(f));
    }
    condition.notify_one();
}

void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] {
                return stop || !tasks.empty();
            });
            if (stop && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task(); // 执行任务
    }
}

// 显式模板实例化
template void ThreadPool::add_task<std::function<void()>>(std::function<void()> &&);
