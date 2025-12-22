#include "Socket.h"
#include <iostream>
#include <unistd.h>     
#include <fcntl.h>    
#include <cstring>       

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
        perror("Socket bind error");
        throw std::runtime_error("Socket bind failed.");
    }    
}

void Socket::listen() {
    int ret = ::listen(fd_, SOMAXCONN);
    if (ret == -1) {
        perror("Socket listen error");
        throw std::runtime_error("Socket listen failed.");
    }
}

int Socket::accept(InetAddress* addr) {
    struct sockaddr_in client_addr;
    socklen_t sock_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(sockaddr_in));

    int clientSocket = ::accept(fd_, (sockaddr*)&client_addr, &sock_len);
    if (clientSocket == -1) {
        perror("Socket accept error");
        return -1;
    }
    if (addr != nullptr) {
        addr->set_addr(client_addr);
    }

    return clientSocket;
}
