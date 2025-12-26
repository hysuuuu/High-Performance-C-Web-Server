#include "Eventloop.h"
#include "Epoll.h"
#include "Channel.h"

Eventloop::Eventloop() : quit_(false) {
    epoll_ = new Epoll();
}

Eventloop::~Eventloop() {
    delete epoll_;
    for (auto& ch : active_ch_) {
        delete ch.second;
    }
}

void Eventloop::loop() {
    while(!quit_) {        
        std::vector<epoll_event> events = epoll_->poll(-1);

        for (auto event : events) {   
            int active_fd = event.data.fd;
            Channel* active_ch = active_ch_[active_fd];
            active_ch->handle_event();
        }
    }
}

void Eventloop::update_channel(Channel* ch) {
    int fd = ch->get_fd();
    active_ch_[fd] = ch;
    epoll_->add_fd(fd, ch->get_events());
}