#pragma once

#include <arpa/inet.h>

#include "Socket.h"
#include "Channel.h"

class InetAddress;
class Channel;
class Eventloop;

class Acceptor {
private:
    Socket sock_;    
    Channel accept_ch_;
    Eventloop* loop_;

    std::function<void(int)> new_connection_callback_;

public:
    Acceptor(const char* ip, uint16_t port, Eventloop* loop);
    ~Acceptor();

    void accept_connection();

    // Setter and Getter
    void set_new_connection_callback(std::function<void(int)> cb) { new_connection_callback_ = cb; }

    const Socket* get_sock() const { return &sock_; }
    const Channel* get_channel() const { return &accept_ch_; }   

};