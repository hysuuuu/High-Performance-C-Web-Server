#include "Buffer.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {
int fail(const char* msg) {
    std::cerr << msg << std::endl;
    return 1;
}

bool expect_eq_size(const char* label, size_t got, size_t expected) {
    if (got != expected) {
        std::cerr << label << " expected " << expected << ", got " << got << std::endl;
        return false;
    }
    return true;
}

bool expect_eq_str(const char* label, const std::string& got, const std::string& expected) {
    if (got != expected) {
        std::cerr << label << " expected '" << expected << "', got '" << got << "'" << std::endl;
        return false;
    }
    return true;
}
}

int main() {
    {
        Buffer b;
        if (!expect_eq_size("initial readable", b.get_readable_bytes(), 0)) {
            return 1;
        }
        if (!expect_eq_size("initial writable", b.get_writable_bytes(), Buffer::k_initial_size)) {
            return 1;
        }
        if (!expect_eq_size("initial prependable", b.get_prependable_bytes(), Buffer::k_cheap_prepend)) {
            return 1;
        }
    }

    {
        Buffer b;
        b.append("abc", 3);
        if (!expect_eq_size("append readable", b.get_readable_bytes(), 3)) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("append content", got, "abc")) {
            return 1;
        }

        b.retrieve(2);
        if (!expect_eq_size("retrieve readable", b.get_readable_bytes(), 1)) {
            return 1;
        }
        std::string got2(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("retrieve content", got2, "c")) {
            return 1;
        }

        b.retrieve_all();
        if (!expect_eq_size("retrieve_all readable", b.get_readable_bytes(), 0)) {
            return 1;
        }
        if (!expect_eq_size("retrieve_all prependable", b.get_prependable_bytes(), Buffer::k_cheap_prepend)) {
            return 1;
        }
    }

    {
        Buffer b(8);
        b.append("12345678", 8);
        b.retrieve(6);
        b.append("abcdef", 6);
        if (!expect_eq_size("compact readable", b.get_readable_bytes(), 8)) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("compact content", got, "78abcdef")) {
            return 1;
        }
    }

    {
        Buffer b;
        b.append("hello", 5);
        std::string got = b.retrieve_all_as_string();
        if (!expect_eq_str("retrieve_all_as_string", got, "hello")) {
            return 1;
        }
        if (!expect_eq_size("retrieve_all_as_string readable", b.get_readable_bytes(), 0)) {
            return 1;
        }
    }

    {
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            return fail("pipe failed");
        }

        const char* msg = "pipe-data";
        if (::write(pipefd[1], msg, std::strlen(msg)) < 0) {
            ::close(pipefd[0]);
            ::close(pipefd[1]);
            return fail("pipe write failed");
        }

        Buffer b;
        int saved_errno = 0;
        ssize_t n = b.read_fd(pipefd[0], &saved_errno);
        ::close(pipefd[0]);
        ::close(pipefd[1]);

        if (n < 0 || saved_errno != 0) {
            return fail("read_fd small read failed");
        }
        if (!expect_eq_size("read_fd small readable", b.get_readable_bytes(), std::strlen(msg))) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("read_fd small content", got, msg)) {
            return 1;
        }
    }

    {
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            return fail("pipe failed");
        }

        std::string msg(40, 'x');
        if (::write(pipefd[1], msg.data(), msg.size()) < 0) {
            ::close(pipefd[0]);
            ::close(pipefd[1]);
            return fail("pipe write failed");
        }

        Buffer b(8);
        int saved_errno = 0;
        ssize_t n = b.read_fd(pipefd[0], &saved_errno);
        ::close(pipefd[0]);
        ::close(pipefd[1]);

        if (n < 0 || saved_errno != 0) {
            return fail("read_fd large read failed");
        }
        if (!expect_eq_size("read_fd large readable", b.get_readable_bytes(), msg.size())) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("read_fd large content", got, msg)) {
            return 1;
        }
    }

    // Test: Write buffer - append string method
    {
        Buffer b;
        std::string data = "Hello, World!";
        b.append(data);
        if (!expect_eq_size("append string readable", b.get_readable_bytes(), data.length())) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("append string content", got, data)) {
            return 1;
        }
    }

    // Test: Write buffer - multiple appends
    {
        Buffer b;
        b.append("Hello", 5);
        b.append(" ", 1);
        b.append("World", 5);
        if (!expect_eq_size("multiple append readable", b.get_readable_bytes(), 11)) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("multiple append content", got, "Hello World")) {
            return 1;
        }
    }

    // Test: Write buffer - append empty data
    {
        Buffer b;
        b.append("", 0);
        if (!expect_eq_size("append empty readable", b.get_readable_bytes(), 0)) {
            return 1;
        }
        b.append("test", 4);
        if (!expect_eq_size("append after empty readable", b.get_readable_bytes(), 4)) {
            return 1;
        }
    }

    // Test: Write buffer - large data append (exceeds initial size)
    {
        Buffer b(16); // Small initial size
        std::string large_data(2048, 'X'); // Larger than initial buffer
        b.append(large_data);
        if (!expect_eq_size("large append readable", b.get_readable_bytes(), large_data.length())) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("large append content", got, large_data)) {
            return 1;
        }
    }

    // Test: Write buffer - ensure_writable_bytes
    {
        Buffer b(16);
        size_t initial_writable = b.get_writable_bytes();
        b.ensure_writable_bytes(100);
        if (b.get_writable_bytes() < 100) {
            return fail("ensure_writable_bytes didn't ensure enough space");
        }
    }

    // Test: Write buffer - begin_write pointer
    {
        Buffer b;
        b.append("abc", 3);
        char* write_ptr = b.begin_write();
        // Write directly to buffer
        const char* data = "def";
        std::copy(data, data + 3, write_ptr);
        // The append updates internal state, but direct write doesn't
        // So we need to test that begin_write returns correct position
        size_t readable = b.get_readable_bytes();
        if (!expect_eq_size("begin_write readable unchanged", readable, 3)) {
            return 1;
        }
    }

    // Test: Write buffer - append after partial retrieve
    {
        Buffer b;
        b.append("abcdef", 6);
        b.retrieve(3); // Remove "abc"
        b.append("ghi", 3);
        if (!expect_eq_size("append after retrieve readable", b.get_readable_bytes(), 6)) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("append after retrieve content", got, "defghi")) {
            return 1;
        }
    }

    // Test: Write buffer - buffer growth strategy
    {
        Buffer b(8);
        // Fill initial space
        b.append("12345678", 8);
        size_t initial_capacity = b.get_writable_bytes() + b.get_readable_bytes() + b.get_prependable_bytes();
        
        // This should trigger growth
        b.append("90", 2);
        if (!expect_eq_size("buffer growth readable", b.get_readable_bytes(), 10)) {
            return 1;
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("buffer growth content", got, "1234567890")) {
            return 1;
        }
    }

    // Test: Write buffer - interleaved read/write operations
    {
        Buffer b;
        b.append("line1\n", 6);
        std::string line1(b.peek(), 6);
        b.retrieve(6);
        
        b.append("line2\n", 6);
        b.append("line3\n", 6);
        
        if (!expect_eq_size("interleaved readable", b.get_readable_bytes(), 12)) {
            return 1;
        }
        
        std::string lines(b.peek(), 12);
        if (!expect_eq_str("interleaved content", lines, "line2\nline3\n")) {
            return 1;
        }
    }

    // Test: Write buffer - writable bytes calculation
    {
        Buffer b(64);
        size_t initial_writable = b.get_writable_bytes();
        if (!expect_eq_size("initial writable correct", initial_writable, 64)) {
            return 1;
        }
        
        b.append("test", 4);
        size_t after_write = b.get_writable_bytes();
        if (!expect_eq_size("writable after write", after_write, 60)) {
            return 1;
        }
    }

    // Test: Write buffer - continuous write until full, then grow
    {
        Buffer b(16);
        for (int i = 0; i < 5; i++) {
            b.append("data", 4);
        }
        if (!expect_eq_size("continuous write readable", b.get_readable_bytes(), 20)) {
            return 1;
        }
        std::string expected(5, ' ');
        for (int i = 0; i < 5; i++) {
            expected.replace(i * 4, 4, "data");
        }
        std::string got(b.peek(), b.get_readable_bytes());
        if (!expect_eq_str("continuous write content", got, "datadatadatadatadata")) {
            return 1;
        }
    }

    std::cout << "Buffer tests passed." << std::endl;
    return 0;
}
