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

    std::cout << "Buffer tests passed." << std::endl;
    return 0;
}
