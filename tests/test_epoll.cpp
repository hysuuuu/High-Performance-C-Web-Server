#include "Epoll.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cassert>
#include <vector>

int main() {
    // Create a pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    // Create Epoll instance
    Epoll ep;
    
    // Add read end of pipe to epoll
    try {
        ep.add_fd(pipefd[0], EPOLLIN);
    } catch (const std::exception& e) {
        std::cerr << "Failed to add fd: " << e.what() << std::endl;
        return 1;
    }

    // Write to write end of pipe
    const char* msg = "hello";
    if (write(pipefd[1], msg, strlen(msg)) == -1) {
        perror("write");
        return 1;
    }

    // Poll for events
    std::vector<epoll_event> events = ep.poll(1000); // 1 second timeout

    // Check if we got the event
    bool event_found = false;
    for (const auto& event : events) {
        if (event.data.fd == pipefd[0] && (event.events & EPOLLIN)) {
            event_found = true;
            break;
        }
    }

    if (!event_found) {
        std::cerr << "Test Failed: Did not receive EPOLLIN event on pipe." << std::endl;
        return 1;
    } else {
        std::cout << "Received EPOLLIN event as expected." << std::endl;
    }

    // Read from pipe to clear the event
    char buf[1024];
    read(pipefd[0], buf, sizeof(buf));

    // Remove fd
    try {
        ep.remove_fd(pipefd[0]);
    } catch (const std::exception& e) {
        std::cerr << "Failed to remove fd: " << e.what() << std::endl;
        return 1;
    }

    // Verify removal
    // If we write again, we shouldn't get an event if it's removed.
    if (write(pipefd[1], msg, strlen(msg)) == -1) {
        perror("write");
        return 1;
    }
    
    events = ep.poll(100); // Short timeout
    for (const auto& event : events) {
        if (event.data.fd == pipefd[0]) {
            std::cerr << "Test Failed: Received event after removal." << std::endl;
            return 1;
        }
    }
    std::cout << "No event received after removal as expected." << std::endl;

    close(pipefd[0]);
    close(pipefd[1]);

    std::cout << "Epoll Test Passed" << std::endl;
    return 0;
}
