#include "Server.h"
#include "Eventloop.h"
#include "Acceptor.h"
#include "Connection.h"

Server::Server(const char* ip, uint16_t port, Eventloop* loop) : loop_(loop), acceptor_(new Acceptor(ip, port, loop_)), thread_pool_(new Threadpool(5)) {
	stop_timer_ = false;
    timer_thread_ = std::thread(&Server::sweep_idle_connections, this);
    
    acceptor_->set_new_connection_callback([this](int fd) {
        this->new_connection(fd);
    });
}

Server::~Server() {
    stop_timer_ = true;
    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }

    delete acceptor_;
    delete thread_pool_;

    connection_map_.clear();
}

void Server::sweep_idle_connections() {
    while (!stop_timer_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::vector<int> expired_fds;
        auto now = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(server_mutex_);
            for (auto& pair : connection_map_) {
                // if last activity time is more than 10 seconds 
                if (now - pair.second->get_last_active_time() > std::chrono::seconds(10)) {
                    expired_fds.push_back(pair.first); 
                }
            }
        }

        for (int fd : expired_fds) {
            std::cout << "[Watchdog] Connection idle timeout, disconnecting fd: " << fd << std::endl;
            delete_connection(fd); 
        }
    }
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