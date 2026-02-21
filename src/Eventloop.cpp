#include "Eventloop.h"
#include "Epoll.h"
#include "Channel.h"

Eventloop::Eventloop() : quit_(false) {
    epoll_ = new Epoll();
}

Eventloop::~Eventloop() {
    delete epoll_;
    // Note: Do NOT delete channels here - they are owned by Connection, not Eventloop
    // Eventloop only stores pointers to channels for event management
    active_ch_.clear();
}

void Eventloop::loop() {
    while(!quit_) {        
        // std::cout << "[Debug] Eventloop: Waiting in epoll_wait..." << std::endl;

        std::vector<epoll_event> events = epoll_->poll(-1);

        // if (!events.empty()) {
        //     std::cout << "[Debug] Eventloop: Got " << events.size() << " events." << std::endl;
        // }

        for (auto event : events) {   
            int active_fd = event.data.fd;
            Channel* active_ch = active_ch_[active_fd];
            // active_ch->handle_event();
            if (active_ch) {
                active_ch->set_revents(event.events);
                active_ch->handle_event();
            }
        }
    }
}

void Eventloop::update_channel(Channel* ch) {
    int fd = ch->get_fd();
    active_ch_[fd] = ch;
    epoll_->add_fd(fd, ch->get_events());
}