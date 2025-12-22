#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

#include "InetAddress.h"

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
    
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;    
};
