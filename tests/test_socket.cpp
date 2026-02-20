#include "Socket.h"
#include "InetAddress.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <atomic>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

// Simple echo server/client test for Socket::bind/listen/accept
int main() {
    constexpr uint16_t PORT = 34567; // choose a high, likely-free port
    std::atomic<bool> server_ready{false};
    std::atomic<bool> server_ok{true};

    // Start server in a background thread
    std::thread server_thread([&](){
        try {
            Socket server;
            InetAddress listen_addr("0.0.0.0", PORT);
            server.bind(listen_addr);
            server.listen();
            server_ready.store(true);

            InetAddress client_addr("0.0.0.0", 0);
            int client_fd = server.accept(&client_addr);
            if (client_fd < 0) {
                std::cerr << "accept failed" << std::endl;
                server_ok.store(false);
                return;
            }

            char buf[1024];
            // Use a timeout on the read to avoid blocking indefinitely
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(client_fd, &readfds);
            
            struct timeval tv;
            tv.tv_sec = 2;  // 2 second timeout
            tv.tv_usec = 0;
            
            int activity = ::select(client_fd + 1, &readfds, nullptr, nullptr, &tv);
            if (activity <= 0) {
                std::cerr << "read timeout or select error" << std::endl;
                server_ok.store(false);
                ::close(client_fd);
                return;
            }
            
            ssize_t n = ::read(client_fd, buf, sizeof(buf));
            if (n > 0) {
                // echo back
                ssize_t m = ::write(client_fd, buf, n);
                if (m != n) {
                    std::cerr << "write size mismatch: wrote " << m << " expected " << n << std::endl;
                    server_ok.store(false);
                }
            } else if (n == 0) {
                std::cerr << "read returned 0 bytes (client closed connection early)" << std::endl;
                server_ok.store(false);
            } else {
                std::cerr << "read failed with errno: " << errno << std::endl;
                server_ok.store(false);
            }
            ::close(client_fd);
        } catch (const std::exception& ex) {
            std::cerr << "Server exception: " << ex.what() << std::endl;
            server_ok.store(false);
        }
    });

    // Wait for server to listen (increased timeout for GitHub Actions)
    for (int i = 0; i < 100 && !server_ready.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    if (!server_ready.load()) {
        std::cerr << "Server did not become ready in time" << std::endl;
        server_thread.join();
        return 1;
    }

    // Give server a bit more time to enter accept() with retry
    int connect_retries = 10;
    int client = -1;
    
    while (connect_retries-- > 0) {
        client = ::socket(AF_INET, SOCK_STREAM, 0);
        if (client == -1) {
            std::cerr << "client socket() failed" << std::endl;
            server_thread.join();
            return 1;
        }

        sockaddr_in srv{};
        srv.sin_family = AF_INET;
        srv.sin_port = htons(PORT);
        if (::inet_pton(AF_INET, "127.0.0.1", &srv.sin_addr) != 1) {
            std::cerr << "inet_pton failed" << std::endl;
            ::close(client);
            server_thread.join();
            return 1;
        }

        if (::connect(client, reinterpret_cast<sockaddr*>(&srv), sizeof(srv)) == 0) {
            break;  // Connection successful
        }
        
        ::close(client);
        client = -1;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    if (client == -1) {
        std::cerr << "connect failed after retries" << std::endl;
        server_thread.join();
        return 1;
    }

    const char* msg = "hello-socket";
    ssize_t sent = ::send(client, msg, std::strlen(msg), 0);
    if (sent != static_cast<ssize_t>(std::strlen(msg))) {
        std::cerr << "send failed or partial: sent " << sent << " expected " << std::strlen(msg) << std::endl;
        ::close(client);
        server_thread.join();
        return 1;
    }

    // Wait for echo with a timeout
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(client, &readfds);
    
    struct timeval tv;
    tv.tv_sec = 2;  // 2 second timeout
    tv.tv_usec = 0;
    
    int activity = ::select(client + 1, &readfds, nullptr, nullptr, &tv);
    if (activity <= 0) {
        std::cerr << "recv timeout or select error" << std::endl;
        ::close(client);
        server_thread.join();
        return 1;
    }

    char echo[1024];
    ssize_t recvd = ::recv(client, echo, sizeof(echo), 0);
    if (recvd <= 0) {
        std::cerr << "recv failed: recvd=" << recvd << " errno=" << errno << std::endl;
        ::close(client);
        server_thread.join();
        return 1;
    }

    std::string echoed(echo, echo + recvd);
    ::close(client);

    server_thread.join();

    if (!server_ok.load()) {
        std::cerr << "Server side failed" << std::endl;
        return 1;
    }

    if (echoed != "hello-socket") {
        std::cerr << "Echo mismatch: " << echoed << std::endl;
        return 1;
    }

    std::cout << "Socket echo test passed." << std::endl;
    return 0;
}
