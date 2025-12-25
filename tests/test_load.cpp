#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const int PORT = 8888;

// 1st Client: The "Blocker", holds connection for 3 seconds
void blocker_client() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        std::cout << "[Blocker] Connected! Sleeping for 3 seconds to block server...\n";
        // Intentionally send no data, causing Blocking Server to get stuck in read()
        // Or send data but don't close, making Server process for a long time
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Send data after waking up to release the Server
        const char* msg = "Blocker done";
        send(sock, msg, strlen(msg), 0);
        close(sock);
    }
}

// Normal Client: Expects immediate response
void normal_client(int id) {
    // Wait a bit to ensure Blocker has connected and blocked the Server
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto start = std::chrono::high_resolution_clock::now();
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        // Connection succeeds (due to OS Backlog), but not yet accepted/processed
        
        const char* msg = "Hello from normal";
        send(sock, msg, strlen(msg), 0);

        char buf[1024];
        // This will block! Because Server is still serving the Blocker
        recv(sock, buf, sizeof(buf), 0); 
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    std::cout << "[Client " << id << "] Request took: " << diff.count() << " seconds.\n";
    close(sock);
}

int main() {
    // 1. Start Blocker to block the Server
    std::thread t1(blocker_client);

    // 2. Start Normal Client (should be fast, but will be delayed by Blocker)
    std::thread t2(normal_client, 1);

    t1.join();
    t2.join();

    return 0;
}