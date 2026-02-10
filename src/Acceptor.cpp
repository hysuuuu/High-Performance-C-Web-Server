#include <unistd.h>

#include "Acceptor.h"
#include "InetAddress.h"
#include "Epoll.h"
#include "Eventloop.h"


Acceptor::Acceptor(const char* ip, uint16_t port, Eventloop* loop) : sock_(), accept_ch_(loop, sock_.get_fd()), loop_(loop) {
    InetAddress server_addr(ip, port);
    
    sock_.set_non_blocking();
    sock_.bind(server_addr);
    sock_.listen();
    std::cout << "Server is listening on port " << port << " ..." << std::endl;

    accept_ch_.set_readCallback([this](){ 
        this->accept_connection(); 
    });
    accept_ch_.enable_reading();
}

Acceptor::~Acceptor() {
    loop_->get_epoll()->remove_fd(accept_ch_.get_fd());                  
}

void Acceptor::accept_connection() {    
    // Prepare an empty address to store client info
    InetAddress client_addr("0.0.0.0", 0);

    while (true) {
        int client_fd = sock_.accept(&client_addr);

        std::cout << "[Debug] Acceptor: New connection event triggered!" << std::endl;

        if (client_fd == -1) {
            // No more connections waiting
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "[Debug] Acceptor: All connections processed (EAGAIN)." << std::endl;
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                perror("Accept error");
                break;
            }
        }   
        
        std::cout << "[Debug] Acceptor: Accepted client fd: " << client_fd << std::endl;
        // Hand to server
        if (new_connection_callback_) {
                new_connection_callback_(client_fd); // Hand to Server
            } else {
                ::close(client_fd); 
        }
    }    
}