#include <unistd.h>
#include <cstring>
#include <string_view>

#include "Socket.h"
#include "Epoll.h"
#include "Channel.h"
#include "Eventloop.h"
#include "Connection.h"

extern "C" {
#include "third_party/picohttpparser/picohttpparser.h"
}


Connection::Connection(int fd, Eventloop* loop) : sock_(new Socket(fd)), loop_(loop), chan_(new Channel(loop, fd)) {
    chan_->set_readCallback([this]() {
        this->handle_read();
    });
    chan_->enable_reading();

    // std::cout << "[Debug] Connection: Constructor called for fd " << fd << std::endl;
}

Connection::~Connection() {
    loop_->get_epoll()->remove_fd(sock_->get_fd());
    delete sock_;  
    delete chan_;
}

void Connection::handle_read() {
    int client_fd = sock_->get_fd();
    int save_errno = 0;
    ssize_t read_bytes;
    while ((read_bytes = read_buffer_.read_fd(client_fd, &save_errno)) > 0) {
        // Read until EAGAIN
    }

    if (read_bytes == 0) {
        std::cout << "[Info] Client disconnected (fd: " << client_fd << ")" << std::endl;
        handle_delete_connection();
        return;
    }

    if (read_bytes == -1 && save_errno != EAGAIN && save_errno != EWOULDBLOCK) {
        perror("Read error");
        handle_delete_connection();
        return;
    }

    const char *method, *path;
    size_t method_len, path_len;
    int minor_version;
    struct phr_header headers[100];
    size_t num_headers = 100;

    // Call pico to parse the request
    int res = phr_parse_request(
        read_buffer_.peek(), read_buffer_.get_readable_bytes(),
        &method, &method_len, &path, &path_len,
        &minor_version, headers, &num_headers, 0
    );

    if (res > 0) { // Parse successful
        std::string_view method_sv(method, method_len);
        std::string_view path_sv(path, path_len);
        
        std::cout << "[HTTP Request] " << method_sv << " " << path_sv << std::endl;

        std::string body = "<html><body><h1>Hello from High-Performance Web Server!</h1></body></html>";
        std::string response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + body;

        write(client_fd, response.data(), response.size());
        read_buffer_.retrieve_all();
        handle_delete_connection();
    } else if (res == -1) {
        std::cerr << "HTTP Parse Error" << std::endl;
        handle_delete_connection();
    }
}

void Connection::handle_delete_connection() {
    if (delete_connection_callback_) {
        delete_connection_callback_(sock_->get_fd());
    }
}



