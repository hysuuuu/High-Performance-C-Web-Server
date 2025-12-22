#include "InetAddress.h"
#include <string>
#include <iostream>
#include <cstring>

InetAddress::InetAddress(const char* ip, uint16_t port) {
    // Clear memory
    memset(&addr_, 0, sizeof(addr_));

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);

    // Check for ip
    if (ip == nullptr || std::string(ip) == "0.0.0.0") {
        addr_.sin_addr.s_addr = INADDR_ANY;
    } else {
        // Convert ip to in_addr_t
        int ret = inet_pton(AF_INET, ip, &addr_.sin_addr);
        if (ret == 0) {
            throw std::runtime_error("Invalid IP address format.");
        } else if (ret < 0) {
            throw std::runtime_error("inet_pton error.");
        }
    }    
}
