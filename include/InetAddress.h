#pragma once

#include <arpa/inet.h>

class InetAddress {

/*
struct sockaddr_in {
    short            sin_family;
    unsigned short   sin_port;
    struct in_addr   sin_addr; 
    char             sin_zero[8];
};

struct in_addr {
    in_addr_t s_addr; 
};
*/

private:
    struct sockaddr_in addr_;
public:
    InetAddress(const char* ip, uint16_t port);
    ~InetAddress() = default;

    // Set addr for accept
    void set_addr(sockaddr_in addr) {
        addr_ = addr;
    }

    // Getter function
    const sockaddr_in* get_addr() const {
        return &addr_;
    }
};
