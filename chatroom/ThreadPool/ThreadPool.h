#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

class ThreadPool {
public:
    // 使用硬件并发数来创建线程池
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // 添加任务到线程池
    template<class F>
    void add_task(F&& f);

private:
    // 工作线程函数
    void worker_thread();

    std::vector<std::thread> threads; // 工作线程
    std::queue<std::function<void()>> tasks; // 任务队列
    std::mutex queue_mutex; // 保护任务队列的互斥锁
    std::condition_variable condition; // 条件变量，用于线程等待和通知
    bool stop; // 停止标志
};

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