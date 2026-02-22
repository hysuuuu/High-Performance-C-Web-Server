#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

class Eventloop;

class EventLoopThread {
private:
    Eventloop* loop_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

public:
    EventLoopThread();
    ~EventLoopThread();

    Eventloop* start_loop();
};

class EventLoopThreadPool {
private:
    Eventloop* main_loop_;
    int num_threads_;
    int next_; // for Round-Robin 
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<Eventloop*> loops_;

public:
    EventLoopThreadPool(Eventloop* main_loop, int num_threads = 5);
    ~EventLoopThreadPool();

    void start();
    Eventloop* get_next_loop(); // Go to next reactor
};