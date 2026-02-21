#include "Threadpool.h"

Threadpool::Threadpool(size_t workers_num) : stop_(false) {
    for (size_t i = 0; i < workers_num; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                // Lock the queue to safely fetch a task
                std::unique_lock<std::mutex> lock(queue_mutex_);

                // Wait until the pool is stopped or there is a task 
                condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) {
                    return;
                }

                auto task = std::move(tasks_.front());  // Fetch the task
                tasks_.pop();
                lock.unlock();  // Unlock mutex
                task(); // Run the task
            }
        });
    }
}

Threadpool::~Threadpool() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
    lock.unlock();
    condition_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }        
    }
}

std::string Threadpool::get_worker_id() {
    std::thread::id this_id = std::this_thread::get_id();
    std::stringstream ss;
    ss << this_id;
    return ss.str();
}