#include "Epoll.h"
#include <iostream>
#include <unistd.h>  

Epoll::Epoll() : epfd_(-1), ep_events_(nullptr) {
    epfd_ = epoll_create1(0);
    if (epfd_ == -1) {
        throw std::runtime_error("Epoll creation failed.");
    }

    ep_events_ = new epoll_event[MAX_EVENTS];
}

Epoll::~Epoll() {
    if (epfd_ != -1) {
        close(epfd_);
        epfd_ = -1;
    }
    if (ep_events_ != nullptr) {
        delete[] ep_events_;
        ep_events_ = nullptr;
    }
}

void Epoll::add_fd(int fd, uint32_t op) {
    if (fd == -1) {
        perror("Epoll fd error");
        throw std::runtime_error("Epoll fd error.");
    }

    struct epoll_event ev;
    ev.events = op;
    ev.data.fd = fd;

    int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret == -1) {
        perror("Epoll add error");
        throw std::runtime_error("Epoll add failed.");
    }
}

void Epoll::remove_fd(int fd) {
    if (fd == -1) {
        perror("Epoll fd error");
        throw std::runtime_error("Epoll fd error.");
    }

    int ret = epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
    if (ret == -1) {
        perror("Epoll del error");
        throw std::runtime_error("Epoll del failed.");
    }
}

std::vector<epoll_event> Epoll::poll(int timeout) {
    int fd_ready = epoll_wait(epfd_, ep_events_, MAX_EVENTS, timeout);
    if (fd_ready == -1) {
        perror("Epoll wait error");
        throw std::runtime_error("Epoll wait failed.");
    }

    std::vector<epoll_event> ready_events;
    if (fd_ready > 0) {
        for (int i = 0; i < fd_ready; ++i) {
            ready_events.push_back(ep_events_[i]);
        }
    }

    return ready_events;
}