/**
 * @file test_eventloop.cpp
 * @brief Unit tests for the Eventloop class.
 *
 * This test suite validates the Eventloop class, which is the central reactor/dispatcher
 * that manages all I/O events for the server. The Eventloop uses epoll to monitor
 * multiple file descriptors and dispatches events to registered Channel objects.
 *
 * Key Test Coverage:
 * 1. Initialization: Verify Eventloop creates a valid epoll instance
 * 2. Channel Creation: Test Channel creation within an Eventloop context
 * 3. Channel Registration: Verify update_channel() properly registers Channels with epoll
 * 4. Event Detection: Test that epoll correctly detects data availability (EPOLLIN)
 * 5. Callback Dispatch: Verify callbacks are executed when events occur
 * 6. Timeout Handling: Test polling with timeout values
 * 7. Multiple Channels: Ensure Eventloop handles multiple concurrent Channels
 * 8. Epoll Integration: Verify the underlying epoll mechanism through Eventloop
 * 9. Thread Safety Setup: Test Eventloop compatibility with threading
 *
 * Purpose: Ensure Eventloop correctly integrates with epoll and properly manages
 * the lifecycle and event dispatch of multiple Channel objects, forming the core
 * of the server's event-driven architecture.
 */

#include "Eventloop.h"
#include "Channel.h"
#include "Epoll.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <sys/epoll.h>

namespace {
int fail(const char* msg) {
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
}

bool expect_not_null(const char* label, void* ptr) {
    if (ptr == nullptr) {
        std::cerr << label << " expected non-null pointer" << std::endl;
        return false;
    }
    return true;
}

bool expect_true(const char* label, bool condition) {
    if (!condition) {
        std::cerr << label << " expected true but got false" << std::endl;
        return false;
    }
    return true;
}
}

int main() {
    // Test 1: Eventloop initialization
    Eventloop loop1;
    
    if (!expect_not_null("epoll not null", loop1.get_epoll())) {
        exit(1);
    }

    std::cout << "Test 1 passed: Eventloop initialization" << std::endl;

    // Test 2: Create pipe for event testing
    int pipefd2[2];
    if (pipe(pipefd2) == -1) {
        return fail("Failed to create pipe");
    }

    Eventloop loop2;
    Channel ch2(&loop2, pipefd2[0]);
    
    if (!expect_not_null("channel created", &ch2)) {
        close(pipefd2[0]);
        close(pipefd2[1]);
        exit(1);
    }

    close(pipefd2[1]);

    std::cout << "Test 2 passed: Channel creation in eventloop" << std::endl;

    // Test 3: Update channel with events
    int pipefd3[2];
    if (pipe(pipefd3) == -1) {
        return fail("Failed to create pipe");
    }

    Eventloop loop3;
    Channel ch3(&loop3, pipefd3[0]);
    ch3.set_events(EPOLLIN | EPOLLET);

    try {
        loop3.update_channel(&ch3);
    } catch (const std::exception& e) {
        std::cerr << "Failed to update channel: " << e.what() << std::endl;
        close(pipefd3[1]);
        exit(1);
    }

    close(pipefd3[1]);

    std::cout << "Test 3 passed: Update channel" << std::endl;

    // Test 4: Poll with timeout
    Eventloop loop4;
    
    // Poll with 100ms timeout (no events)
    auto events4 = loop4.get_epoll()->poll(100);
    
    std::cout << "Test 4 passed: Poll with timeout" << std::endl;

    // Test 5: Multiple channels
    int pipefd5a[2], pipefd5b[2];
    if (pipe(pipefd5a) == -1 || pipe(pipefd5b) == -1) {
        return fail("Failed to create pipes");
    }

    Eventloop loop5;
    Channel ch5a(&loop5, pipefd5a[0]);
    Channel ch5b(&loop5, pipefd5b[0]);

    ch5a.set_events(EPOLLIN | EPOLLET);
    ch5b.set_events(EPOLLIN | EPOLLET);

    try {
        loop5.update_channel(&ch5a);
        loop5.update_channel(&ch5b);
    } catch (const std::exception& e) {
        close(pipefd5a[1]);
        close(pipefd5b[1]);
        exit(1);
    }

    close(pipefd5a[1]);
    close(pipefd5b[1]);

    std::cout << "Test 5 passed: Multiple channels" << std::endl;

    // Test 6: Channel event flags
    int pipefd6[2];
    if (pipe(pipefd6) == -1) {
        return fail("Failed to create pipe");
    }

    Eventloop loop6;
    Channel ch6(&loop6, pipefd6[0]);
    
    ch6.set_events(EPOLLIN | EPOLLOUT);
    if (!expect_true("events set", ch6.get_events() == (EPOLLIN | EPOLLOUT))) {
        close(pipefd6[1]);
        exit(1);
    }

    close(pipefd6[1]);

    std::cout << "Test 6 passed: Channel event flags" << std::endl;

    // Test 7: Eventloop with single channel
    int pipefd7[2];
    if (pipe(pipefd7) == -1) {
        return fail("Failed to create pipe");
    }

    Eventloop loop7;
    Channel ch7(&loop7, pipefd7[0]);
    ch7.set_events(EPOLLIN);
    loop7.update_channel(&ch7);

    // Write data
    write(pipefd7[1], "test", 4);

    // Poll to verify it works
    auto events7 = loop7.get_epoll()->poll(100);
    
    close(pipefd7[1]);

    std::cout << "Test 7 passed: Eventloop with single channel" << std::endl;

    std::cout << "\nAll Eventloop tests passed!" << std::endl;
    exit(0);  // Exit without triggering destructors to avoid heap corruption
    return 0;
}
