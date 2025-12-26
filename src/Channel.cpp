#include <sys/epoll.h>

#include "Channel.h"
#include "Eventloop.h"

void Channel::enable_reading() {
    set_events(EPOLLIN | EPOLLET);
    loop_->update_channel(this);
}

void Channel::handle_event() {
    if (revents_ & EPOLLIN) {
        if (read_callback_) read_callback_();
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_) write_callback_();
    }
}