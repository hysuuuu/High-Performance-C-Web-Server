#include "Socket.h"
#include "InetAddress.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <atomic>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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
            ssize_t n = ::read(client_fd, buf, sizeof(buf));
            if (n > 0) {
                // echo back
                ssize_t m = ::write(client_fd, buf, n);
                if (m != n) {
                    std::cerr << "write size mismatch" << std::endl;
                    server_ok.store(false);
                }
            } else {
                std::cerr << "read failed or zero bytes" << std::endl;
                server_ok.store(false);
            }
            ::close(client_fd);
        } catch (const std::exception& ex) {
            std::cerr << "Server exception: " << ex.what() << std::endl;
            server_ok.store(false);
        }
    });

    // Wait briefly for server to listen
    for (int i = 0; i < 50 && !server_ready.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Create client socket and connect
    int client = ::socket(AF_INET, SOCK_STREAM, 0);
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

    if (::connect(client, reinterpret_cast<sockaddr*>(&srv), sizeof(srv)) == -1) {
        std::perror("connect");
        ::close(client);
        server_thread.join();
        return 1;
    }

    const char* msg = "hello-socket";
    ssize_t sent = ::send(client, msg, std::strlen(msg), 0);
    if (sent != static_cast<ssize_t>(std::strlen(msg))) {
        std::cerr << "send failed or partial" << std::endl;
        ::close(client);
        server_thread.join();
        return 1;
    }

    char echo[1024];
    ssize_t recvd = ::recv(client, echo, sizeof(echo), 0);
    if (recvd <= 0) {
        std::cerr << "recv failed" << std::endl;
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
