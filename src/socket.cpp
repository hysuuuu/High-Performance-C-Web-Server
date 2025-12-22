#include "Socket.h"
#include <iostream>
#include <unistd.h>     
#include <fcntl.h>           

Socket::Socket() : fd_(-1) {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1) {
        throw std::runtime_error("Socket creation failed.");
    }

    // Set port reusable
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

Socket::~Socket() {
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

void Socket::bind(const InetAddress& addr) {
    int ret = ::bind(fd_, (struct sockaddr*)addr.get_addr(), sizeof(sockaddr_in));
    if (ret == -1) {
        perror("socket bind error");
        throw std::runtime_error("Socket bind failed.");
    }    
}

void Socket::listen() {

}

int Socket::accept(InetAddress* addr) {

}
