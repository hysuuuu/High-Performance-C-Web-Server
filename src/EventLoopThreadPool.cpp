#include "EventLoopThreadPool.h"
#include "Eventloop.h"

EventLoopThread::EventLoopThread() : loop_(nullptr) {}

EventLoopThread::~EventLoopThread() {
    if (loop_) {
        loop_->quit();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

Eventloop* EventLoopThread::start_loop() {
    thread_ = std::thread([this]() {
        Eventloop loop;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            loop_ = &loop;
        }
        cond_.notify_one();
        
        loop.loop(); 
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            loop_ = nullptr;
        }
    });

    Eventloop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return loop_ != nullptr; });
        loop = loop_;
    }
    return loop;
}


EventLoopThreadPool::EventLoopThreadPool(Eventloop* main_loop, int num_threads)
    : main_loop_(main_loop), num_threads_(num_threads), next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start() {
    for (int i = 0; i < num_threads_; ++i) {
        auto t = std::make_unique<EventLoopThread>();
        loops_.push_back(t->start_loop()); 
        threads_.push_back(std::move(t));
    }
}

Eventloop* EventLoopThreadPool::get_next_loop() {
    if (loops_.empty()) {
        return main_loop_;
    }
    // Round-Robin 
    Eventloop* loop = loops_[next_];
    next_ = (next_ + 1) % num_threads_;
    return loop;
}