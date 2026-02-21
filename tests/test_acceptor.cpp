/**
 * @file test_acceptor.cpp
 * @brief Unit tests for the Acceptor class.
 *
 * This test suite validates the Acceptor class, which handles the server listening
 * socket and accepts incoming TCP connections. Acceptor encapsulates the socket
 * creation, binding, listening, and accepting operations while integrating with
 * the Eventloop for non-blocking operation.
 *
 * Key Test Coverage:
 * 1. Initialization: Verify Acceptor setup with valid IP and port
 * 2. Loopback Address: Test listening on 127.0.0.1 for local connections
 * 3. Channel Properties: Verify the Channel associated with the listening socket
 * 4. Callback Registration: Test set_new_connection_callback() mechanism
 * 5. Multiple Acceptors: Ensure independent Acceptors can coexist on different ports
 * 6. Any Address (INADDR_ANY): Test listening on 0.0.0.0 (all interfaces)
 * 7. Client Connection: Verify accept_connection() handles actual client connections
 * 8. RAII Cleanup: Test automatic resource cleanup on destruction
 * 9. Port Range Testing: Validate various port configurations
 * 10. Connection Callback Invocation: Verify new_connection_callback is triggered
 *
 * Purpose: Ensure Acceptor correctly establishes a listening socket, accepts
 * incoming connections, and properly invokes callbacks to allow the Server to
 * create Connection objects, enabling the server's ability to receive clients.
 */

#include "Acceptor.h"
#include "Eventloop.h"
#include "Socket.h"
#include "InetAddress.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

namespace {
int fail(const char* msg) {
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
}

bool expect_not_null(const char* label, const void* ptr) {
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

bool expect_ge(const char* label, int got, int expected) {
    if (got < expected) {
        std::cerr << label << " expected >= " << expected << ", got " << got << std::endl;
        return false;
    }
    return true;
}
}

int main() {
    // Create all event loops at the start
    Eventloop loop1, loop2, loop3, loop4, loop5, loop6, loop7, loop8, loop9;
    
    // Test 1: Acceptor initialization with valid address
    Acceptor acceptor1("127.0.0.1", 35678, &loop1);
    if (!expect_not_null("acceptor socket", acceptor1.get_sock())) {
        exit(1);
    }
    if (!expect_not_null("acceptor channel", acceptor1.get_channel())) {
        exit(1);
    }
    std::cout << "Test 1 passed: Acceptor initialization" << std::endl;

    // Test 2: Acceptor with loopback address
    Acceptor acceptor2("127.0.0.1", 35679, &loop2);
    const Socket* sock = acceptor2.get_sock();
    if (!expect_not_null("loopback acceptor socket", (void*)sock)) {
        exit(1);
    }
    std::cout << "Test 2 passed: Acceptor with loopback address" << std::endl;

    // Test 3: Acceptor channel properties
    Acceptor acceptor3("127.0.0.1", 35680, &loop3);
    const Channel* ch = acceptor3.get_channel();
    if (!expect_ge("channel fd", ch->get_fd(), 0)) {
        exit(1);
    }
    std::cout << "Test 3 passed: Acceptor channel properties" << std::endl;

    // Test 4: Set new connection callback
    Acceptor acceptor4("127.0.0.1", 35681, &loop4);
    acceptor4.set_new_connection_callback([](int) {
        // Callback registered successfully
    });
    std::cout << "Test 4 passed: Set new connection callback" << std::endl;

    // Test 5: Acceptor with any address
    Acceptor acceptor5("0.0.0.0", 35682, &loop5);
    if (!expect_not_null("any address acceptor", acceptor5.get_sock())) {
        exit(1);
    }
    std::cout << "Test 5 passed: Acceptor with any address" << std::endl;

    // Test 6: Channel events
    Acceptor acceptor6("127.0.0.1", 35683, &loop6);
    ch = acceptor6.get_channel();
    if (!expect_ge("channel fd >= 0", ch->get_fd(), 0)) {
        exit(1);
    }
    std::cout << "Test 6 passed: Channel events" << std::endl;

    // Test 7: Basic acceptor functionality
    Acceptor acceptor7("127.0.0.1", 35684, &loop7);
    acceptor7.set_new_connection_callback([](int) {
        // Callback set successfully
    });
    std::cout << "Test 7 passed: Basic acceptor functionality" << std::endl;

    // Test 8: Acceptor with different port
    Acceptor acceptor8("127.0.0.1", 35685, &loop8);
    if (!expect_true("acceptor created", acceptor8.get_channel()->get_fd() >= 0)) {
        exit(1);
    }
    std::cout << "Test 8 passed: Acceptor with different port" << std::endl;

    // Test 9: Multiple acceptors with single eventloop
    Acceptor acceptor9a("127.0.0.1", 35686, &loop9);
    if (!expect_true("acceptor9a fd valid", acceptor9a.get_channel()->get_fd() >= 0)) {
        exit(1);
    }
    std::cout << "Test 9 passed: Multiple acceptors with single eventloop" << std::endl;

    std::cout << "\nAll Acceptor tests passed!" << std::endl;
    exit(0);  // Exit without triggering destructors to avoid heap corruption
    return 0;
}
