#include <unistd.h>
#include <cstring>

#include "Socket.h"
#include "Epoll.h"
#include "Channel.h"
#include "Eventloop.h"
#include "Connection.h"


Connection::Connection(int fd, Eventloop* loop) : sock_(new Socket(fd)), loop_(loop), chan_(new Channel(loop, fd)) {
    chan_->set_readCallback([this]() {
        this->handle_read();
    });
    chan_->enable_reading();
}

Connection::~Connection() {
    loop_->get_epoll()->remove_fd(sock_->get_fd());
    delete sock_;  
    delete chan_;
}

void Connection::handle_read() {
    int client_fd = sock_->get_fd();
    char buffer[1024];
    int read_bytes = read(client_fd, buffer, sizeof(buffer)); 

    while (read_bytes > 0) {
        std::cout << "Received: " << buffer << std::endl;
        read_bytes = read(client_fd, buffer, sizeof(buffer)); 
        
        // Simple greeting (write to socket)
        const char* hello = "Hello from Server!\n";
        write(client_fd, hello, strlen(hello));            
    }
    if (read_bytes == 0) {
        // Close client connection and remove from epoll
        std::cout << "Client disconnected.";      
        if (delete_connection_callback_) {
            delete_connection_callback_(client_fd);
        }
    } else if (read_bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
        } else {
            perror("Read error");    
            if (delete_connection_callback_) {
                delete_connection_callback_(client_fd);
            }          
        }                      
    }       
}



