#pragma once

#include <functional>
#include <string>
#include <mutex>
#include <memory>
#include <chrono>

#include "Buffer.h"

class Socket;
class Channel;
class Eventloop;
class Threadpool;

/**
 * @class Connection
 * @brief Encapsulates the state and processing logic of a single TCP connection (The Waiter).
 *
 * Each connected client corresponds to one Connection object.
 * This class uses RAII to manage resources: the Socket and Channel are created 
 * upon construction and cleaned up upon destruction.
 *
 * Key Responsibilities:
 * 1. Manages the client's Socket fd and its associated Channel.
 * 2. Handles read/write events (e.g., handle_read).
 * 3. Executes business logic (e.g., Echoing data back).
 * 4. Detects remote disconnection (read returns 0) and notifies the Server 
 * to destroy this object.
 */
class Connection : public std::enable_shared_from_this<Connection> {
private:
    Eventloop* loop_;
    Socket* sock_;
    Channel* chan_;

    Buffer read_buffer_;
    Buffer write_buffer_;

    bool is_disconnecting_;

    std::function<void(int)> delete_connection_callback_;

    Threadpool* pool_;
    std::mutex conn_mutex_;

    std::chrono::steady_clock::time_point last_active_time_;

public:
    Connection(int fd, Eventloop* loop, Threadpool* pool);
    ~Connection();

    void update_active_time() {
        last_active_time_ = std::chrono::steady_clock::now();
    }
    std::chrono::steady_clock::time_point get_last_active_time() const {
        return last_active_time_;
    }

    void handle_read();
    void handle_write();

    void send(const std::string& msg);
    void disconnect();

    void process_request();

    void handle_delete_connection();  
    void set_delete_connection_callback(std::function<void(int)> cb) { delete_connection_callback_ = cb; }   

};