#pragma once

#include <arpa/inet.h>
#include <map>

class Eventloop;
class Socket;
class Channel;
class Acceptor;
class Connection;

class Server {
private:
    Eventloop* loop_;
    Acceptor* acceptor_;

    std::map<int, Connection*> connection_map_;

public:
    Server(const char* ip, uint16_t port, Eventloop* loop);
    ~Server();

    void new_connection(int fd);
    void delete_connection(int fd);
};