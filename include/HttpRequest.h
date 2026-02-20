#pragma once

#include <string>
#include <vector>
#include <string_view>

struct HttpHeader {
    std::string_view name;
    std::string_view value;
};

class HttpRequest {
public:
    std::string_view method;
    std::string_view path;
    int version;
    std::vector<HttpHeader> headers;

    void clear() {
        method = path = "";
        version = 0;
        headers.clear();
    }
};