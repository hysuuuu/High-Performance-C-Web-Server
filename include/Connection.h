#pragma once

#include <functional>

class Socket;
class Channel;
class Eventloop;
class Server;

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