#pragma once

#include <map>

class Epoll;
class Channel;

class Eventloop {
private:
    Epoll* epoll_;
    std::map<int, Channel*> active_ch_;
    bool quit_;

public:  
    Eventloop();
    ~Eventloop();

    void loop();
    void update_channel(Channel* ch);

    // Getter and Setter
    Epoll* get_epoll() const { return epoll_; }
};