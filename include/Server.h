#pragma once

#include <arpa/inet.h>
#include <map>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>

#include "Threadpool.h"

class Eventloop;
class Socket;
class Channel;
class Acceptor;
class Connection;
class EventLoopThreadPool;

/**
 * @class Server
 * @brief The core management class of the server (The Manager).
 *
 * The Server class acts as the central hub. It owns the EventLoop and the Acceptor,
 * and it manages the lifecycle of all active connections.
 *
 * Key Responsibilities:
 * 1. Owns the Acceptor to start listening on a specific port.
 * 2. Maintains a map (connection_map_) of all active connections.
 * 3. Handles new connection creation (new_connection): Creates a Connection object.
 * 4. Handles connection destruction (delete_connection): Removes the Connection 
 * from the map and releases resources.
 */
class Server {
private:
    Eventloop* loop_;
    Acceptor* acceptor_;

    EventLoopThreadPool* sub_reactor_pool_; 

    Threadpool* thread_pool_;
    std::mutex server_mutex_;

    std::map<int, std::shared_ptr<Connection>> connection_map_;

    std::thread timer_thread_;
    std::atomic<bool> stop_timer_;

    void sweep_idle_connections();

public:
    Server(const char* ip, uint16_t port, Eventloop* loop);
    ~Server();

    void new_connection(int fd);
    void delete_connection(int fd);
};