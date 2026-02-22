#pragma once

#include <map>
#include <mutex>

class Epoll;
class Channel;

class Eventloop {
private:
    Epoll* epoll_;
    std::map<int, Channel*> active_ch_;
    bool quit_;
    std::mutex loop_mutex_; // for active_ch

public:  
    Eventloop();
    ~Eventloop();

    void loop();
    void update_channel(Channel* ch);
    void remove_channel(Channel* ch);
    
    void quit() {quit_ = true; }

    // Getter and Setter
    Epoll* get_epoll() const { return epoll_; }
};