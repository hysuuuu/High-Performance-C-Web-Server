#include "Server.h"
#include "Eventloop.h"
#include "Acceptor.h"
#include "Connection.h"

Server::Server(const char* ip, uint16_t port, Eventloop* loop) : loop_(loop), acceptor_(new Acceptor(ip, port, loop_)) {
	acceptor_->set_new_connection_callback([this](int fd) {
        this->new_connection(fd);
    });
}

void Server::new_connection(int fd) {
    Connection* connection = new Connection(fd, loop_);
    connection_map_[fd] = connection;    

    connection->set_delete_connection_callback([this](int fd) {
        this->delete_connection(fd);
    });
}

void Server::delete_connection(int fd) {
    auto it = connection_map_.find(fd);
    if (it != connection_map_.end()) {
        delete it->second;
        connection_map_.erase(it);
    }    
}