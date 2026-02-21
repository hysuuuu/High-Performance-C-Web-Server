#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <type_traits>
#include <string>
#include <sstream>

/**
 * @class ThreadPool
 * @brief Manages a pool of worker threads to execute tasks asynchronously.
 * * Uses a thread-safe task queue. Worker threads wait on a condition variable
 * until a task is available, reducing CPU usage when idle.
 */

class Threadpool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    std::mutex queue_mutex_;
    std::condition_variable condition_;

    bool stop_;

public:
    Threadpool(size_t workers_num);
    ~Threadpool();


    /**
     * @brief Add a new task to the thread pool's queue and return a future.
     * @tparam F The function type
     * @tparam Args The argument types
     */
    template<class F, class... Args>
    auto add_task(F&& f, Args&&... args) 
        -> std::future<std::result_of_t<F(Args...)>> {
        
        // Get the return type
        using return_type = std::result_of_t<F(Args...)>;

        auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
            
        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("add_task on stopped Threadpool");
            }
            
            tasks_.emplace([task]() { 
                (*task)(); 
            });
        }
        
        condition_.notify_one();
        
        return res; 
    }

    // Getter
    std::string get_worker_id();
 };