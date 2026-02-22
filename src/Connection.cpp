#include <unistd.h>
#include <cstring>
#include <string_view>
#include <fstream>
#include <sstream>

#include "Socket.h"
#include "Epoll.h"
#include "Channel.h"
#include "Eventloop.h"
#include "Connection.h"
#include "HttpResponse.h"
#include "Threadpool.h"
#include "Server.h"

extern "C" {
#include "third_party/picohttpparser/picohttpparser.h"
}

std::string get_mime_type(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) return "image/jpeg";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".ico") != std::string::npos) return "image/x-icon";
    return "text/plain";
}

Connection::Connection(int fd, Eventloop* loop, Threadpool* pool, Server* server) 
        : loop_(loop), sock_(new Socket(fd)), chan_(new Channel(loop, fd)), 
            read_buffer_(), write_buffer_(), is_disconnecting_(false), pool_(pool),
            server_(server) {
    update_active_time();
    // Register read and write callbacks
    chan_->set_readCallback([this]() { this->handle_read(); });
    chan_->set_writeCallback([this]() { this->handle_write(); });
    chan_->enable_reading();
}

Connection::~Connection() {
    loop_->remove_channel(chan_);
    delete sock_;  
    delete chan_;
}

void Connection::handle_read() {    
    int client_fd = sock_->get_fd();
    int save_errno = 0;
    ssize_t read_bytes;
    bool has_read_data = false;

    while ((read_bytes = read_buffer_.read_fd(client_fd, &save_errno)) > 0) {
        // Read until EAGAIN
        has_read_data = true;
    }

    if (has_read_data) {
        update_active_time();

        auto entry = wheel_entry_.lock();
        if (entry && server_) {
            server_->update_connection_timer(entry);
        }
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


    if (read_buffer_.get_readable_bytes() > 0) {
        // std::string raw_request = read_buffer_.retrieve_all_as_string();
        
        auto self = shared_from_this();        
        pool_->add_task([self]() {
            self->process_request();
        });
    }
}

void Connection::process_request() {
    while (true) {
        std::string request_data;
        {
            std::lock_guard<std::mutex> lock(conn_mutex_);
            if (read_buffer_.get_readable_bytes() == 0) return;
            request_data = std::string(read_buffer_.peek(), read_buffer_.get_readable_bytes());
        }

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
            {
                std::lock_guard<std::mutex> lock(conn_mutex_);
                read_buffer_.retrieve(res); 
            }

            bool keep_alive = (minor_version == 1); 
            for (size_t i = 0; i < num_headers; ++i) {
                std::string_view name(headers[i].name, headers[i].name_len);
                std::string_view value(headers[i].value, headers[i].value_len);
                if (name == "Connection" || name == "connection") {
                    if (value == "close" || value == "Close") keep_alive = false;
                    if (value == "keep-alive" || value == "Keep-Alive") keep_alive = true;
                }
            }

            std::string req_path(path, path_len);
            size_t query_pos = req_path.find('?');
            if (query_pos != std::string::npos) {
                req_path = req_path.substr(0, query_pos);
            }
            if (req_path == "/") {
                req_path = "/index.html"; 
            }
            std::string file_path = "../www" + req_path;

            HttpResponse res_obj(!keep_alive);
            std::ifstream file(file_path, std::ios::binary);

            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf(); 
                res_obj.set_status_code(HttpResponse::k200Ok);
                res_obj.set_status_message("OK");
                res_obj.set_content_type(get_mime_type(file_path));
                res_obj.set_body(buffer.str());
            } else {
                res_obj.set_status_code(HttpResponse::k404NotFound);
                res_obj.set_status_message("Not Found");
                res_obj.set_content_type("text/html");
                res_obj.set_body("<html><body><h1>404 Not Found</h1></body></html>");
            }

            send(res_obj.to_string());

            if (!keep_alive) {
                disconnect();
                break;
            }
        } else if (res == -2) {
            // Incomplete request
            break;
        } else {
            // HTTP format error
            std::cerr << "HTTP Parse Error" << std::endl;
            disconnect();
            break;
        }
    }    
}

void Connection::handle_write() {
    update_active_time();
    {
        auto entry = wheel_entry_.lock();
        if (entry && server_) {
            server_->update_connection_timer(entry);
        }
    }
    bool should_disconnect = false;
    
    {
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

    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        is_disconnecting_ = true;
        // If we are not waiting to send remaining data, we can disconnect immediately
        if (!chan_->is_writing()) {
            should_disconnect = true;
        }
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



