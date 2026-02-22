#include "Server.h"
#include "Eventloop.h"
#include "Acceptor.h"
#include "Connection.h"
#include "EventLoopThreadPool.h"

Entry::Entry(const std::weak_ptr<Connection>& conn) : conn_(conn) {}

Entry::~Entry() {
    auto conn = conn_.lock();
    if (conn) {
        conn->disconnect();
    }
}

Server::Server(const char* ip, uint16_t port, Eventloop* loop) : loop_(loop), acceptor_(new Acceptor(ip, port, loop_)), sub_reactor_pool_(new EventLoopThreadPool(loop_, 5)) , thread_pool_(new Threadpool(5)) {
	sub_reactor_pool_->start();
    
    wheel_.resize(10);
    stop_timer_ = false;

    timer_thread_ = std::thread([this]() {
        while (!stop_timer_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            this->tick();
        }
    });
    
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
    delete sub_reactor_pool_;

    connection_map_.clear();
}

void Server::tick() {
    std::lock_guard<std::mutex> lock(wheel_mutex_);

    wheel_.push_back(Bucket());

    Bucket& timed_out_bucket = wheel_.front();
    
    // Traverse Entry, if the connection does not appear in other bucket, then timeout
    for (const auto& entryPtr : timed_out_bucket) {
        if (entryPtr.use_count() == 1) {
            std::cout << "[Watchdog] Timing Wheel detected timeout" << std::endl; 
        }
    }

    wheel_.pop_front();
}

void Server::new_connection(int fd) {
    std::lock_guard<std::mutex> lock(server_mutex_);    // Lock to protect map

    Eventloop* sub_loop = sub_reactor_pool_->get_next_loop();

    auto connection = std::make_shared<Connection>(fd, sub_loop, thread_pool_, this);
    connection_map_[fd] = connection;    

    {
        std::lock_guard<std::mutex> w_lock(wheel_mutex_);
        EntryPtr entry = std::make_shared<Entry>(connection);
        wheel_.back().insert(entry);
        connection->set_wheel_entry(entry);
    }

    connection->set_delete_connection_callback([this](int fd) {
        this->delete_connection(fd);
    });
}

void Server::delete_connection(int fd) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    connection_map_.erase(fd);
}

void Server::update_connection_timer(const WeakEntryPtr& weak_entry) {
    EntryPtr entry = weak_entry.lock();
    if (entry) {
        std::lock_guard<std::mutex> lock(wheel_mutex_);
        wheel_.back().insert(entry); 
    }
}