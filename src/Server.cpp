#include "Server.h"
#include "Eventloop.h"
#include "Acceptor.h"
#include "Connection.h"

Server::Server(const char* ip, uint16_t port, Eventloop* loop) : loop_(loop), acceptor_(new Acceptor(ip, port, loop_)), thread_pool_(new Threadpool(5)) {
	acceptor_->set_new_connection_callback([this](int fd) {
        this->new_connection(fd);
    });
}

Server::~Server() {
    delete acceptor_;
    delete thread_pool_;

    for (auto& item : connection_map_) {
        delete item.second; 
    }
}

void Server::new_connection(int fd) {
    std::lock_guard<std::mutex> lock(server_mutex_);    // Lock to protect map

    Connection* connection = new Connection(fd, loop_, thread_pool_);
    connection_map_[fd] = connection;    

    connection->set_delete_connection_callback([this](int fd) {
        this->delete_connection(fd);
    });
}

void Server::delete_connection(int fd) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    auto it = connection_map_.find(fd);
    if (it != connection_map_.end()) {
        delete it->second;
        connection_map_.erase(it);
    }    
}