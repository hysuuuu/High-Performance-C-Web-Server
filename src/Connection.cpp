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
    int save_errno = 0;
    ssize_t read_bytes = read_buffer_.read_fd(client_fd, &save_errno);

    while (read_bytes > 0) {
        std::string msg = read_buffer_.retrieve_all_as_string();
        std::cout << "Received: " << msg << std::endl;
        
        // Simple greeting (write to socket)
        const char* hello = "Hello from Server!\n";
        // TODO: change to Write Buffer
        write(client_fd, hello, strlen(hello));      
        
        read_bytes = read_buffer_.read_fd(client_fd, &save_errno);
    }
    if (read_bytes == 0) {
        // Close client connection and remove from epoll
        std::cout << "Client disconnected.";      
        if (delete_connection_callback_) {
            delete_connection_callback_(client_fd);
        }
    } else if (read_bytes == -1) {
        if (save_errno == EAGAIN || save_errno == EWOULDBLOCK) {
        } else {
            perror("Read error");    
            if (delete_connection_callback_) {
                delete_connection_callback_(client_fd);
            }          
        }                      
    }       
}



