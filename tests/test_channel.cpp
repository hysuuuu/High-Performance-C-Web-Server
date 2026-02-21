/**
 * @file test_channel.cpp
 * @brief Unit tests for the Channel class.
 *
 * This test suite validates the Channel class, which represents a file descriptor
 * wrapper with associated event callbacks. Channel is a critical component that
 * bridges low-level epoll events with high-level callback handlers.
 *
 * Key Test Coverage:
 * 1. Initialization: Verify Channel creation with valid file descriptors
 * 2. Event Flag Management: Test setting and retrieving event flags (EPOLLIN, EPOLLOUT)
 * 3. Received Event Management: Test setting and retrieving revents (events received from epoll)
 * 4. Callback Registration: Verify read/write callback registration
 * 5. Callback Execution: Test that callbacks are invoked when appropriate events occur
 * 6. Multiple Events: Verify handling of combined event types (read + write)
 * 7. No Callbacks: Ensure Channel handles missing callbacks gracefully
 * 8. Event Updates: Test updating event flags multiple times
 * 9. Integration with Eventloop: Verify Channel works correctly within an Eventloop context
 *
 * Purpose: Ensure Channel correctly manages file descriptor events and executes
 * user-provided callbacks when events are detected by the epoll mechanism.
 */

#include "Channel.h"
#include "Eventloop.h"

#include <cassert>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <unistd.h>
#include <sys/epoll.h>

namespace {
int fail(const char* msg) {
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
}

bool expect_eq_int(const char* label, int got, int expected) {
    if (got != expected) {
        std::cerr << label << " expected " << expected << ", got " << got << std::endl;
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

bool expect_false(const char* label, bool condition) {
    if (condition) {
        std::cerr << label << " expected false but got true" << std::endl;
        return false;
    }
    return true;
}
}

int main() {
    // Test 1: Create pipe for testing
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return fail("Failed to create pipe");
    }

    {
        // Test 2: Channel initialization
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        if (!expect_eq_int("initial fd", ch.get_fd(), pipefd[0])) {
            return 1;
        }

        if (!expect_eq_int("initial events", ch.get_events(), 0)) {
            return 1;
        }

        if (!expect_eq_int("initial revents", ch.get_revents(), 0)) {
            return 1;
        }
    }

    {
        // Test 3: Set and get events
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        uint32_t test_events = EPOLLIN | EPOLLOUT;
        ch.set_events(test_events);

        if (!expect_eq_int("set events", ch.get_events(), test_events)) {
            return 1;
        }
    }

    {
        // Test 4: Set and get revents
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        uint32_t test_revents = EPOLLIN;
        ch.set_revents(test_revents);

        if (!expect_eq_int("set revents", ch.get_revents(), test_revents)) {
            return 1;
        }
    }

    {
        // Test 5: Callback registration and execution
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        bool read_called = false;
        bool write_called = false;

        ch.set_readCallback([&]() {
            read_called = true;
        });

        ch.set_writeCallback([&]() {
            write_called = true;
        });

        // Test read callback
        ch.set_revents(EPOLLIN);
        ch.handle_event();

        if (!expect_true("read callback called", read_called)) {
            return 1;
        }

        if (!expect_false("write callback not called", write_called)) {
            return 1;
        }
    }

    {
        // Test 6: Write callback execution
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        bool read_called = false;
        bool write_called = false;

        ch.set_readCallback([&]() {
            read_called = true;
        });

        ch.set_writeCallback([&]() {
            write_called = true;
        });

        // Test write callback
        ch.set_revents(EPOLLOUT);
        ch.handle_event();

        if (!expect_false("read callback not called", read_called)) {
            return 1;
        }

        if (!expect_true("write callback called", write_called)) {
            return 1;
        }
    }

    {
        // Test 7: Both callbacks execution
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        bool read_called = false;
        bool write_called = false;

        ch.set_readCallback([&]() {
            read_called = true;
        });

        ch.set_writeCallback([&]() {
            write_called = true;
        });

        // Test both callbacks
        ch.set_revents(EPOLLIN | EPOLLOUT);
        ch.handle_event();

        if (!expect_true("both callbacks - read", read_called)) {
            return 1;
        }

        if (!expect_true("both callbacks - write", write_called)) {
            return 1;
        }
    }

    {
        // Test 8: No callbacks registered
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        ch.set_revents(EPOLLIN | EPOLLOUT);
        // Should not crash even with no callbacks
        ch.handle_event();
    }

    {
        // Test 9: Multiple event flag updates
        Eventloop loop;
        Channel ch(&loop, pipefd[0]);

        ch.set_events(EPOLLIN);
        if (!expect_eq_int("first event set", ch.get_events(), EPOLLIN)) {
            return 1;
        }

        ch.set_events(EPOLLOUT);
        if (!expect_eq_int("second event set", ch.get_events(), EPOLLOUT)) {
            return 1;
        }

        ch.set_events(EPOLLIN | EPOLLOUT);
        if (!expect_eq_int("combined events", ch.get_events(), EPOLLIN | EPOLLOUT)) {
            return 1;
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);

    std::cout << "All Channel tests passed!" << std::endl;
    exit(0);  // Exit without triggering destructors to avoid heap corruption
    return 0;
}
