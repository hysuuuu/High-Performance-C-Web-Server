#include "Eventloop.h"
#include "Epoll.h"
#include "Channel.h"
#include "Connection.h"

#include <iostream>

Eventloop::Eventloop() : quit_(false), last_tick_(std::chrono::steady_clock::now()) {
    epoll_ = new Epoll();
    wheel_.resize(10);
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

        std::vector<epoll_event> events = epoll_->poll(1000);

        for (auto event : events) {   
            int active_fd = event.data.fd;
            // Channel* active_ch = active_ch_[active_fd];
            Channel* active_ch = nullptr;

            {
                std::lock_guard<std::mutex> lock(loop_mutex_);
                if (active_ch_.count(active_fd) > 0) {
                    active_ch = active_ch_[active_fd];
                }
            }

            if (active_ch) {
                active_ch->set_revents(event.events);
                active_ch->handle_event();
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_tick_ >= std::chrono::seconds(1)) {
            tick();
            last_tick_ = now;
        }
    }
}

void Eventloop::tick() {
    wheel_.push_back(Bucket());

    Bucket& timed_out_bucket = wheel_.front();
    for (const auto& entryPtr : timed_out_bucket) {
        if (entryPtr.use_count() == 1) {
            std::cout << "[Watchdog] Timing Wheel detected timeout" << std::endl;
        }
    }

    wheel_.pop_front();
}

WeakEntryPtr Eventloop::add_connection_timer(const std::shared_ptr<Connection>& conn) {
    EntryPtr entry = std::make_shared<Entry>(conn);
    wheel_.back().insert(entry);
    return WeakEntryPtr(entry);
}

void Eventloop::update_connection_timer(const WeakEntryPtr& weak_entry) {
    EntryPtr entry = weak_entry.lock();
    if (entry) {
        wheel_.back().insert(entry);
    }
}

void Eventloop::tick_once_for_test() {
    tick();
}

void Eventloop::update_channel(Channel* ch) {
    int fd = ch->get_fd();
    active_ch_[fd] = ch;
    epoll_->add_fd(fd, ch->get_events());
}

void Eventloop::remove_channel(Channel* ch) {
    int fd = ch->get_fd();
    {
        std::lock_guard<std::mutex> lock(loop_mutex_);
        active_ch_.erase(fd);        
    }
    epoll_->remove_fd(fd);
}