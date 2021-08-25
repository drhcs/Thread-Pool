#ifndef THREAD_POOL
#define THREAD_POOL

#include <functional>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

template <class T>
class ThreadPool {

    const int kThreads_;

    std::vector<std::thread> pool_;
    std::queue<T> jobs_;
    std::function<void(T)> DoJob_;

    std::mutex queue_mutex_;
    std::condition_variable worker_;
    std::atomic<bool> pool_active_;

    void DequeueJob() {

        while (true)
        {
            T job;
            {
                std::unique_lock<std::mutex> ul(queue_mutex_);
                while (pool_active_ && jobs_.empty())
                {
                    worker_.wait(ul);
                }

                if (!pool_active_)
                {
                    return;
                }

                job = jobs_.front();
                jobs_.pop();
            }

            DoJob_(job);
        }
    }

public:

    explicit ThreadPool(std::function<void(T)> DoJob, int kThreads = std::thread::hardware_concurrency()) : DoJob_(DoJob),  kThreads_(kThreads) {
        pool_active_ = true;
        for (int i = 0; i < kThreads_; i++)
        {
            pool_.push_back(std::thread(&ThreadPool::DequeueJob, this));
        }
    }

    ~ThreadPool() {
        pool_active_ = false;
        worker_.notify_all();
        for (auto & worker : pool_)
        {
            worker.join();
        }
    }

    void EnqueueJob(T job) {
        {
            std::unique_lock<std::mutex> ul(queue_mutex_);
            jobs_.push(job);
            worker_.notify_one();
        }
    }

};

#endif
