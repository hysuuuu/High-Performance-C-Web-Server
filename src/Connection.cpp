#include <unistd.h>
#include <cstring>
#include <string_view>

#include "Socket.h"
#include "Epoll.h"
#include "Channel.h"
#include "Eventloop.h"
#include "Connection.h"
#include "HttpResponse.h"
#include "Threadpool.h"

extern "C" {
#include "third_party/picohttpparser/picohttpparser.h"
}


Connection::Connection(int fd, Eventloop* loop, Threadpool* pool) 
    : loop_(loop), sock_(new Socket(fd)), chan_(new Channel(loop, fd)), 
      read_buffer_(), write_buffer_(), is_disconnecting_(false), pool_(pool) {
    // Register read and write callbacks
    chan_->set_readCallback([this]() { this->handle_read(); });
    chan_->set_writeCallback([this]() { this->handle_write(); });
    chan_->enable_reading();
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


    std::string raw_request = read_buffer_.retrieve_all_as_string();        
    pool_->add_task([this, raw_request]() {
            this->process_request(raw_request);
    });

}

void Connection::process_request(std::string request_data) {
    const char *method, *path;
    size_t method_len, path_len;
    int minor_version;
    struct phr_header headers[100];
    size_t num_headers = 100;

    // Call pico to parse the request
    int res = phr_parse_request(
        request_data.data(), request_data.size(),
        &method, &method_len, &path, &path_len,
        &minor_version, headers, &num_headers, 0
    );

    if (res > 0) { // Parse successful
        HttpResponse res_obj(true);
        res_obj.set_status_code(HttpResponse::k200Ok);
        res_obj.set_status_message("OK");
        res_obj.set_content_type("text/html");
        std::string body = "<html><body><h1>Hello from High-Performance Web Server!</h1></body></html>";
        // big chunk for buffer testing
        // body += std::string(100000, 'A');
        res_obj.set_body(body);

        // Convert response object to string
        std::string response_str = res_obj.to_string();
        send(response_str);
        if (res_obj.is_close_connection()) {
            disconnect();
        }
    } else if (res == -1) {
        std::cerr << "HTTP Parse Error" << std::endl;
        disconnect();
    }
}


void Connection::handle_write() {
    bool should_disconnect = false;
    std::lock_guard<std::mutex> lock(conn_mutex_);

    if (chan_->is_writing()) {
        ssize_t n = write(sock_->get_fd(), write_buffer_.peek(), write_buffer_.get_readable_bytes());
        
        if (n > 0) {
            write_buffer_.retrieve(n); // Remove sent data from buffer
            
            if (write_buffer_.get_readable_bytes() == 0) {
                // Buffer is empty, stop listening to EPOLLOUT
                chan_->disable_writing();
                
                // If the connection was marked for disconnect, close it now
                if (is_disconnecting_) {
                    should_disconnect = true;
                }
            }
        } else {
            perror("Connection handle_write error");
        }
    }

    if (should_disconnect) {
        handle_delete_connection();
    }
}

void Connection::send(const std::string& msg) {
    if (is_disconnecting_) return;

    std::lock_guard<std::mutex> lock(conn_mutex_);

    ssize_t nwrote = 0;
    size_t remaining = msg.size();
    bool fault_error = false;

    // If write buffer is empty, try to write directly to the socket first
    if (!chan_->is_writing() && write_buffer_.get_readable_bytes() == 0) {
        nwrote = write(sock_->get_fd(), msg.data(), msg.size());
        
        if (nwrote >= 0) {
            remaining = msg.size() - nwrote;
            if (remaining == 0 && is_disconnecting_) {
                // Completely sent and connection needs to be closed
                handle_delete_connection();
            }
        } else {
            nwrote = 0;
            // EAGAIN means the system buffer is full, it's normal in non-blocking I/O
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("Connection send error");
                fault_error = true;
            }
        }
    }

    // If there is still remaining data, append it to the write buffer
    if (!fault_error && remaining > 0) {
        write_buffer_.append(msg.data() + nwrote, remaining);
        
        // Start listening to EPOLLOUT to trigger handle_write() later
        if (!chan_->is_writing()) {
            chan_->enable_writing();
        }
    }
}

void Connection::disconnect() {
    bool should_disconnect = false;
    std::lock_guard<std::mutex> lock(conn_mutex_);
    is_disconnecting_ = true;
    // If we are not waiting to send remaining data, we can disconnect immediately
    if (!chan_->is_writing()) {
        should_disconnect = true;
    }

    // Disconnect after unlock
    if (should_disconnect) {
        delete_connection_callback_(sock_->get_fd());
    }
}

void Connection::handle_delete_connection() {
    if (delete_connection_callback_) {
        delete_connection_callback_(sock_->get_fd());
    }
}



