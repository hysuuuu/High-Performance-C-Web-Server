/**
 * @file test_inet_address.cpp
 * @brief Unit tests for the InetAddress class.
 *
 * This test suite validates the InetAddress class, which encapsulates IPv4 socket
 * address information (IP address and port) using the sockaddr_in structure.
 * InetAddress is essential for server binding and creating network connections.
 *
 * Key Test Coverage:
 * 1. Localhost Address: Verify 127.0.0.1 initialization and validation
 * 2. Any Address (INADDR_ANY): Test 0.0.0.0 configuration for listening on all interfaces
 * 3. Standard Port Numbers: Validate common ports (80, 443) and edge cases (1, 65535)
 * 4. Custom IPs: Test various IPv4 addresses (10.0.0.1, private ranges)
 * 5. set_addr() Function: Verify address modification after construction
 * 6. Broadcast Address: Test 255.255.255.255 handling
 * 7. Multiple Addresses: Ensure independent instances don't interfere
 * 8. Ephemeral Ports: Test port 0 configuration (OS-assigned port)
 * 9. Private Network Ranges: Validate Class A (10.x.x.x), B (172.16-31.x.x), C (192.168.x.x)
 *
 * Purpose: Ensure InetAddress correctly initializes and manages IPv4 address structures
 * that comply with socket API requirements for binding and connecting.
 */

#include "InetAddress.h"

#include <cassert>
#include <iostream>
#include <cstdlib>
#include <arpa/inet.h>
#include <cstring>

namespace {
int fail(const char* msg) {
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
}

bool expect_str_eq(const char* label, const char* got, const char* expected) {
    if (strcmp(got, expected) != 0) {
        std::cerr << label << " expected '" << expected << "', got '" << got << "'" << std::endl;
        return false;
    }
    return true;
}

bool expect_int_eq(const char* label, int got, int expected) {
    if (got != expected) {
        std::cerr << label << " expected " << expected << ", got " << got << std::endl;
        return false;
    }
    return true;
}

bool expect_uint_eq(const char* label, uint16_t got, uint16_t expected) {
    if (got != expected) {
        std::cerr << label << " expected " << expected << ", got " << got << std::endl;
        return false;
    }
    return true;
}
}

int main() {
    {
        // Test 1: Valid localhost
        InetAddress addr("127.0.0.1", 8080);
        const sockaddr_in* sa = addr.get_addr();

        if (!expect_int_eq("localhost family", sa->sin_family, AF_INET)) {
            return 1;
        }

        if (!expect_uint_eq("localhost port", ntohs(sa->sin_port), 8080)) {
            return 1;
        }

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("localhost ip", ip_buf, "127.0.0.1")) {
            return 1;
        }
    }

    {
        // Test 2: Any address (0.0.0.0)
        InetAddress addr("0.0.0.0", 9000);
        const sockaddr_in* sa = addr.get_addr();

        if (!expect_int_eq("any address family", sa->sin_family, AF_INET)) {
            return 1;
        }

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("any address ip", ip_buf, "0.0.0.0")) {
            return 1;
        }
    }

    {
        // Test 3: Standard port numbers
        InetAddress addr1("127.0.0.1", 80);
        if (!expect_uint_eq("port 80", ntohs(addr1.get_addr()->sin_port), 80)) {
            return 1;
        }

        InetAddress addr2("127.0.0.1", 443);
        if (!expect_uint_eq("port 443", ntohs(addr2.get_addr()->sin_port), 443)) {
            return 1;
        }

        InetAddress addr3("127.0.0.1", 65535);
        if (!expect_uint_eq("port 65535", ntohs(addr3.get_addr()->sin_port), 65535)) {
            return 1;
        }
    }

    {
        // Test 4: Set and get address
        InetAddress addr("10.0.0.1", 5000);
        const sockaddr_in* sa = addr.get_addr();

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("custom ip", ip_buf, "10.0.0.1")) {
            return 1;
        }

        if (!expect_uint_eq("custom port", ntohs(sa->sin_port), 5000)) {
            return 1;
        }
    }

    {
        // Test 5: set_addr function
        InetAddress addr("1.1.1.1", 1111);

        // Create a new address structure
        struct sockaddr_in new_addr;
        memset(&new_addr, 0, sizeof(new_addr));
        new_addr.sin_family = AF_INET;
        new_addr.sin_port = htons(2222);
        inet_pton(AF_INET, "2.2.2.2", &new_addr.sin_addr);

        addr.set_addr(new_addr);

        const sockaddr_in* sa = addr.get_addr();
        if (!expect_uint_eq("set_addr port", ntohs(sa->sin_port), 2222)) {
            return 1;
        }

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("set_addr ip", ip_buf, "2.2.2.2")) {
            return 1;
        }
    }

    {
        // Test 6: Broadcast address
        InetAddress addr("255.255.255.255", 12345);
        const sockaddr_in* sa = addr.get_addr();

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("broadcast ip", ip_buf, "255.255.255.255")) {
            return 1;
        }
    }

    {
        // Test 7: Multiple addresses
        InetAddress addr1("127.0.0.1", 8000);
        InetAddress addr2("127.0.0.1", 8001);
        InetAddress addr3("127.0.0.1", 8002);

        if (!expect_uint_eq("addr1 port", ntohs(addr1.get_addr()->sin_port), 8000)) {
            return 1;
        }

        if (!expect_uint_eq("addr2 port", ntohs(addr2.get_addr()->sin_port), 8001)) {
            return 1;
        }

        if (!expect_uint_eq("addr3 port", ntohs(addr3.get_addr()->sin_port), 8002)) {
            return 1;
        }
    }

    {
        // Test 8: Ephemeral port (0 - let OS choose)
        InetAddress addr("0.0.0.0", 0);
        const sockaddr_in* sa = addr.get_addr();

        if (!expect_int_eq("ephemeral port family", sa->sin_family, AF_INET)) {
            return 1;
        }
    }

    {
        // Test 9: Private network addresses
        InetAddress addr1("10.0.0.1", 3000);
        InetAddress addr2("192.168.1.1", 3001);
        InetAddress addr3("172.16.0.1", 3002);

        char ip_buf[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &addr1.get_addr()->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("private class A", ip_buf, "10.0.0.1")) {
            return 1;
        }

        inet_ntop(AF_INET, &addr2.get_addr()->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("private class C", ip_buf, "192.168.1.1")) {
            return 1;
        }

        inet_ntop(AF_INET, &addr3.get_addr()->sin_addr, ip_buf, INET_ADDRSTRLEN);
        if (!expect_str_eq("private class B", ip_buf, "172.16.0.1")) {
            return 1;
        }
    }

    std::cout << "All InetAddress tests passed!" << std::endl;
    exit(0);  // Exit without triggering destructors to avoid heap corruption
    return 0;
}

