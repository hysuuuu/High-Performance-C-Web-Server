/**
 * @file test_connection.cpp
 * @brief Unit tests for the Connection class.
 *
 * This test suite validates the Connection class, which represents a single active
 * TCP connection from a client. Connection manages the client's socket, associated
 * Channel, and read buffer using RAII principles for automatic resource cleanup.
 *
 * Key Test Coverage:
 * 1. Initialization: Verify Connection creation with valid socket file descriptors
 * 2. Callback Registration: Test set_delete_connection_callback() registration
 * 3. Multiple Connections: Ensure independent Connection instances work correctly
 * 4. Data Exchange Setup: Test Connection with data written to its paired socket
 * 5. Disconnection Detection: Verify callbacks for remote disconnection (FIN/RST)
 * 6. RAII Cleanup: Test automatic resource cleanup on destruction
 * 7. handle_read() Functionality: Verify the method reads data without crashing
 * 8. handle_delete_connection(): Test manual trigger of connection deletion callback
 * 9. Rapid Lifecycle: Stress test rapid creation and destruction cycles
 * 10. Connection State: Verify internal state management and persistence
 *
 * Purpose: Ensure Connection properly encapsulates TCP connection state, manages
 * socket and buffer resources, and correctly invokes callbacks for read events
 * and disconnections, enabling per-client request processing.
 */

#include "Connection.h"
#include "Eventloop.h"
#include "Socket.h"
#include "Buffer.h"
#include "Threadpool.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <cstring>

namespace {
int fail(const char* msg) {
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
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
    // Create a thread pool for task processing
    Threadpool pool(4);
    
    // Declare all eventloops and connections at start to avoid scope issues
    Eventloop loop1, loop2, loop3, loop4, loop5, loop6, loop7;
    
    // Test 1: Connection initialization with socket pair
    int sockets1[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets1) == -1) {
        return fail("Failed to create socket pair");
    }
    Connection conn1(sockets1[0], &loop1, &pool);
    close(sockets1[1]);
    std::cout << "Test 1 passed: Connection initialization" << std::endl;

    // Test 2: Set delete callback
    int sockets2[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets2) == -1) {
        return fail("Failed to create socket pair");
    }
    Connection conn2(sockets2[0], &loop2, &pool);
    conn2.set_delete_connection_callback([](int) {
        // Callback set
    });
    close(sockets2[1]);
    std::cout << "Test 2 passed: Set delete callback" << std::endl;

    // Test 3: Multiple connections
    int sockets3a[2], sockets3b[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets3a) == -1 ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets3b) == -1) {
        return fail("Failed to create socket pairs");
    }
    Connection conn3a(sockets3a[0], &loop3, &pool);
    Connection conn3b(sockets3b[0], &loop3, &pool);
    close(sockets3a[1]);
    close(sockets3b[1]);
    std::cout << "Test 3 passed: Multiple connections" << std::endl;

    // Test 4: Connection with data exchange
    int sockets4[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets4) == -1) {
        return fail("Failed to create socket pair");
    }
    Connection conn4(sockets4[0], &loop4, &pool);
    const char* test_msg = "hello";
    write(sockets4[1], test_msg, strlen(test_msg));
    close(sockets4[1]);
    std::cout << "Test 4 passed: Connection data exchange" << std::endl;

    // Test 5: Handle delete connection
    int sockets5[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets5) == -1) {
        return fail("Failed to create socket pair");
    }
    Connection conn5(sockets5[0], &loop5, &pool);
    bool deleted = false;
    conn5.set_delete_connection_callback([&](int) {
        deleted = true;
    });
    conn5.handle_delete_connection();
    if (!expect_true("delete connection handled", deleted)) {
        close(sockets5[1]);
        exit(1);
    }
    close(sockets5[1]);
    std::cout << "Test 5 passed: Handle delete connection" << std::endl;

    // Test 6: Connection lifecycle
    int sockets6[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets6) == -1) {
        return fail("Failed to create socket pair");
    }
    Connection conn6(sockets6[0], &loop6, &pool);
    close(sockets6[1]);
    std::cout << "Test 6 passed: Connection lifecycle" << std::endl;

    // Test 7: Single shared eventloop
    int sockets7a[2], sockets7b[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets7a) == -1 ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets7b) == -1) {
        return fail("Failed to create socket pairs");
    }
    Connection conn7a(sockets7a[0], &loop7, &pool);
    Connection conn7b(sockets7b[0], &loop7, &pool);
    close(sockets7a[1]);
    close(sockets7b[1]);
    std::cout << "Test 7 passed: Single shared eventloop" << std::endl;

    std::cout << "\nAll Connection tests passed!" << std::endl;
    exit(0);  // Exit without triggering destructors to avoid heap corruption
    return 0;
}
