/**
 * @file test_timing_wheel.cpp
 * @brief Unit tests for per-Eventloop timing wheel behavior.
 */

#include "Eventloop.h"
#include "Connection.h"
#include "Threadpool.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

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
    std::cout << "Starting Timing Wheel Tests..." << std::endl;

    Threadpool pool(1);
    Eventloop loop;

    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
        return fail("Failed to create socket pair");
    }

    auto conn = std::make_shared<Connection>(sockets[0], &loop, &pool);
    conn->set_delete_connection_callback([](int) {});
    auto weak_entry = loop.add_connection_timer(conn);
    conn->set_wheel_entry(weak_entry);

    if (!expect_true("Entry should exist after add", weak_entry.lock() != nullptr)) {
        close(sockets[1]);
        return 1;
    }

    for (int i = 0; i < 10; ++i) {
        loop.tick_once_for_test();
    }

    if (!expect_true("Entry should expire after full wheel rotation", weak_entry.lock() == nullptr)) {
        close(sockets[1]);
        return 1;
    }

    close(sockets[1]);
    std::cout << "Timing wheel test passed!" << std::endl;
    return 0;
}
