#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <chrono>

#include "TimingWheel.h"

class Epoll;
class Channel;
class Connection;

class Eventloop {
private:
    Epoll* epoll_;
    std::map<int, Channel*> active_ch_;
    bool quit_;
    std::mutex loop_mutex_; // for active_ch

    TimingWheel wheel_;
    std::chrono::steady_clock::time_point last_tick_;

    void tick();

public:  
    Eventloop();
    ~Eventloop();

    void loop();
    void update_channel(Channel* ch);
    void remove_channel(Channel* ch);

    WeakEntryPtr add_connection_timer(const std::shared_ptr<Connection>& conn);
    void update_connection_timer(const WeakEntryPtr& weak_entry);
    void tick_once_for_test();
    
    void quit() {quit_ = true; }

    // Getter and Setter
    Epoll* get_epoll() const { return epoll_; }
};