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

    connection_map_.clear();

}

void Server::new_connection(int fd) {
    std::lock_guard<std::mutex> lock(server_mutex_);    // Lock to protect map

    auto connection = std::make_shared<Connection>(fd, loop_, thread_pool_);
    connection_map_[fd] = connection;    

    connection->set_delete_connection_callback([this](int fd) {
        this->delete_connection(fd);
    });
}

void Server::delete_connection(int fd) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    connection_map_.erase(fd);
}