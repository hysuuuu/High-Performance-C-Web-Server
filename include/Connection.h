#pragma once

#include <functional>

class Socket;
class Channel;
class Eventloop;
class Server;

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
class Connection {
private:
    Eventloop* loop_;
    Socket* sock_;
    Channel* chan_;

    std::function<void(int)> delete_connection_callback_;

public:
    Connection(int fd, Eventloop* loop);
    ~Connection();

    void handle_read();
    void handle_delete_connection();

    void set_delete_connection_callback(std::function<void(int)> cb) { delete_connection_callback_ = cb; }

};