#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

namespace {
const int kPort = 8888;

int connect_with_retry(const char* ip, int port, int attempts, int delay_ms) {
    for (int i = 0; i < attempts; ++i) {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            return -1;
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        inet_pton(AF_INET, ip, &addr.sin_addr);

        if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            return sock;
        }

        ::close(sock);
        ::usleep(delay_ms * 1000);
    }
    return -1;
}

std::string recv_with_timeout(int fd, int timeout_ms) {
    std::string data;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        int ret = ::select(fd + 1, &readfds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(fd, &readfds)) {
            char buf[1024];
            ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
            if (n > 0) {
                data.append(buf, static_cast<size_t>(n));
            } else if (n == 0) {
                break;
            } else if (errno != EINTR) {
                break;
            }
        }
    }

    return data;
}

bool wait_for_substring(int fd, const char* needle, int timeout_ms) {
    std::string data = recv_with_timeout(fd, timeout_ms);
    return data.find(needle) != std::string::npos;
}

pid_t start_server() {
    pid_t pid = ::fork();
    if (pid == 0) {
        ::execl("./server", "server", nullptr);
        _exit(127);
    }
    return pid;
}

void stop_server(pid_t pid) {
    if (pid <= 0) {
        return;
    }
    ::kill(pid, SIGINT);
    int status = 0;
    ::waitpid(pid, &status, 0);
}
}

int main() {
    pid_t server_pid = start_server();
    if (server_pid < 0) {
        std::cerr << "Failed to fork server." << std::endl;
        return 1;
    }

    int sock = connect_with_retry("127.0.0.1", kPort, 50, 50);
    if (sock == -1) {
        std::cerr << "Failed to connect to server." << std::endl;
        stop_server(server_pid);
        return 1;
    }

    // Send proper HTTP GET request
    const char* http_request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    ssize_t sent = ::send(sock, http_request, std::strlen(http_request), 0);
    if (sent != static_cast<ssize_t>(std::strlen(http_request))) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        ::close(sock);
        stop_server(server_pid);
        return 1;
    }

    // Read the full HTTP response once
    std::string response = recv_with_timeout(sock, 1000);
    
    // Verify HTTP status
    if (response.find("HTTP/1.1 200 OK") == std::string::npos) {
        std::cerr << "Did not receive HTTP 200 response." << std::endl;
        std::cerr << "Response: " << response << std::endl;
        ::close(sock);
        stop_server(server_pid);
        return 1;
    }

    // Verify response contains HTML body
    if (response.find("C++ Web Server") == std::string::npos) {
        std::cerr << "Did not receive expected response body." << std::endl;
        std::cerr << "Response: " << response << std::endl;
        ::close(sock);
        stop_server(server_pid);
        return 1;
    }

    ::close(sock);
    stop_server(server_pid);
    return 0;
}
