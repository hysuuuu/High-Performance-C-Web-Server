#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

class InetAddress;
class Socket {
private:
    int fd_;

public:
    Socket();
    ~Socket();

    void bind(const InetAddress& addr);
    void listen();
    int accept(InetAddress* addr);

    // Set to non-blocking for epoll
    void set_non_blocking();

    // Getter function
    int get_fd() const {
        return fd_;
    }
    
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;    
};
