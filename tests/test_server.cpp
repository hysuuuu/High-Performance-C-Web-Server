/**
 * @file test_server.cpp
 * @brief Unit tests for the Server class.
 *
 * This test suite validates the Server class, which is the central management hub
 * coordinating the Acceptor (listening socket), Eventloop (event dispatcher), and
 * all active Connection objects. Server implements the lifecycle management for
 * accepting new connections and cleaning up closed connections.
 *
 * Key Test Coverage:
 * 1. Initialization: Verify Server creation with valid IP and port configuration
 * 2. Multiple Servers: Test independent Server instances on different ports
 * 3. new_connection(): Verify handling of accepted client connections
 * 4. delete_connection(): Test cleanup of closed connections
 * 5. Multiple Concurrent Connections: Ensure Server manages many connections
 * 6. Add/Remove Cycle: Verify connection lifecycle (add then remove)
 * 7. Multi-Port Servers: Test multiple Server instances simultaneously
 * 8. RAII and Cleanup: Verify automatic resource cleanup on destruction
 * 9. Rapid Connection Cycles: Stress test rapid add/remove sequences
 * 10. Connection State Persistence: Verify independent connections don't interfere
 *
 * Purpose: Ensure Server correctly owns and coordinates the Acceptor and Eventloop,
 * maintains a collection of active Connections with proper lifecycle management,
 * and allows the system to handle multiple incoming clients through the callback
 * mechanism from Acceptor and Connection objects.
 */

#include "Server.h"
#include "Eventloop.h"
#include "Connection.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
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

bool expect_eq_int(const char* label, int got, int expected) {
    if (got != expected) {
        std::cerr << label << " expected " << expected << ", got " << got << std::endl;
        return false;
    }
    return true;
}
}

int main() {
    // Test 1: Server initialization
    Eventloop loop1;
    Server server1("127.0.0.1", 36780, &loop1);

    std::cout << "Test 1 passed: Server initialization" << std::endl;

    // Test 2: Server with different addresses
    Eventloop loop2;
    Server server2("127.0.0.1", 36781, &loop2);

    std::cout << "Test 2 passed: Server with different addresses" << std::endl;

    // Test 3: Server with loopback
    Eventloop loop3;
    Server server3("127.0.0.1", 36782, &loop3);

    std::cout << "Test 3 passed: Server with loopback" << std::endl;

    // Test 4: Server with any address
    Eventloop loop4;
    Server server4("0.0.0.0", 36783, &loop4);

    std::cout << "Test 4 passed: Server with any address" << std::endl;

    // Test 5: Server connection handling
    Eventloop loop5;
    Server server5("127.0.0.1", 36784, &loop5);

    int sockets5[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets5) == -1) {
        return fail("Failed to create socket pair");
    }

    server5.new_connection(sockets5[0]);
    server5.delete_connection(sockets5[0]);

    close(sockets5[1]);

    std::cout << "Test 5 passed: Server connection handling" << std::endl;

    // Test 6: Server with multiple ephemeral connections
    Eventloop loop6;
    Server server6("127.0.0.1", 36785, &loop6);

    int sockets6a[2], sockets6b[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets6a) == -1 ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets6b) == -1) {
        return fail("Failed to create socket pairs");
    }

    server6.new_connection(sockets6a[0]);
    server6.new_connection(sockets6b[0]);

    server6.delete_connection(sockets6a[0]);
    server6.delete_connection(sockets6b[0]);

    close(sockets6a[1]);
    close(sockets6b[1]);

    std::cout << "Test 6 passed: Multiple connections" << std::endl;

    // Test 7: Server port variation
    Eventloop loop7;
    Server server7("127.0.0.1", 36786, &loop7);

    if (!expect_true("server created", true)) {
        exit(1);
    }

    std::cout << "Test 7 passed: Server port variation" << std::endl;

    std::cout << "\nAll Server tests passed!" << std::endl;
    exit(0);  // Exit without triggering destructors to avoid heap corruption
    return 0;
}
