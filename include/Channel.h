#pragma once

#include <iostream>
#include <cstdint>
#include <functional>

class Epoll;

class Channel {
private:
    Epoll* epoll_;
    int fd_;
    uint32_t events_;   // Events sent to epoll
    uint32_t revents_;  // Events recieved from epoll

    std::function<void()> read_callback_;
    std::function<void()> write_callback_;

public:
    Channel(Epoll* epoll, int fd) : epoll_(epoll), fd_(fd), events_(0), revents_(0) {};
    ~Channel() = default;

    void enable_reading();
    void handle_event();  

    // Setter 
    void set_events(uint32_t ev) { events_ = ev; }
    void set_revents(uint32_t ev) { revents_ = ev; }    
    void set_readCallback(std::function<void()> cb) { read_callback_ = cb; }
    void set_writeCallback(std::function<void()> cb) { write_callback_ = cb; }

    // Getter 
    int get_fd() const { return fd_; }
    uint32_t get_events() const { return events_; }
    uint32_t get_revents() const { return revents_; }
};