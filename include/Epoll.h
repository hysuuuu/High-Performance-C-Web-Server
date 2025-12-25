#pragma once

#include <sys/epoll.h>
#include <vector>

/*
struct epoll_event {
    uint32_t      events; 
    epoll_data_t  data;  
};

union epoll_data {
    void     *ptr;
    int       fd;
    uint32_t  u32;
    uint64_t  u64;
};
*/

static const int MAX_EVENTS = 1024;

class Epoll {
private:
    int epfd_;
    struct epoll_event* ep_events_;

public:
    Epoll();
    ~Epoll();

    void add_fd(int fd, uint32_t op);
    void remove_fd(int fd);
    std::vector<epoll_event> poll(int timeout);

    // Getter function
    int get_epfd() const {
        return epfd_;
    }
};