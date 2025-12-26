#include "Channel.h"
#include "Epoll.h"

void Channel::enable_reading() {
    set_events(EPOLLIN | EPOLLET);
    epoll_->add_fd(fd_, events_);
}

void Channel::handle_event() {
    if (revents_ & EPOLLIN) {
        if (read_callback_) read_callback_();
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_) write_callback_();
    }
}