/**
 * @brief Unit tests for HTTP request and response protocol handling.
 * * This test suite ensures the integrity of the HTTP application layer:
 * * 1. HttpRequest Validation:
 * - Verifies correct storage of HTTP methods, paths, and versions.
 * - Validates the header management system (insertion and retrieval).
 * - Ensures the clear() function properly resets the object state for connection reuse.
 * * 2. HttpResponse Validation (Protocol Compliance):
 * - Status Line: Confirms correct formatting for various status codes (200, 301, 400, 404).
 * - Header Automation: Tests automatic generation of 'Content-Length' and 'Connection' headers based on state.
 * - Payload Integrity: Validates handling of empty, large (10KB+), and special-character-laden bodies.
 * - Wire Format: Ensures the critical CRLF (\r\n\r\n) separator between headers and body is strictly maintained.
 * * Purpose: To guarantee that the server generates and interprets data in strict accordance 
 * with the HTTP/1.1 specification, preventing protocol-level errors in browsers.
 */

#include "HttpRequest.h"
#include "HttpResponse.h"

#include <cassert>
#include <iostream>
#include <string>

namespace {

int fail(const char* msg) {
    std::cerr << "FAIL: " << msg << std::endl;
    return 1;
}

bool expect_eq_str(const char* label, const std::string& got, const std::string& expected) {
    if (got != expected) {
        std::cerr << label << " expected '" << expected << "', got '" << got << "'" << std::endl;
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

bool expect_eq_bool(const char* label, bool got, bool expected) {
    if (got != expected) {
        std::cerr << label << " expected " << (expected ? "true" : "false") << ", got " << (got ? "true" : "false") << std::endl;
        return false;
    }
    return true;
}

bool expect_eq_size(const char* label, size_t got, size_t expected) {
    if (got != expected) {
        std::cerr << label << " expected " << expected << ", got " << got << std::endl;
        return false;
    }
    return true;
}

bool contains_substring(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

int main() {
    // ================== HttpRequest Tests ==================
    {
        std::cout << "Testing HttpRequest::clear()..." << std::endl;
        HttpRequest req;
        req.method = "GET";
        req.path = "/index.html";
        req.version = 11;
        req.headers.push_back({"Content-Type", "text/html"});
        
        if (!expect_eq_size("headers before clear", req.headers.size(), 1)) {
            return 1;
        }
        
        req.clear();
        
        if (!expect_eq_size("headers after clear", req.headers.size(), 0)) {
            return 1;
        }
        if (!expect_eq_int("version after clear", req.version, 0)) {
            return 1;
        }
    }

    {
        std::cout << "Testing HttpRequest with headers..." << std::endl;
        HttpRequest req;
        req.method = "POST";
        req.path = "/api/submit";
        req.version = 11;
        
        req.headers.push_back({"Content-Type", "application/json"});
        req.headers.push_back({"Content-Length", "42"});
        req.headers.push_back({"Host", "example.com"});
        
        if (!expect_eq_size("header count", req.headers.size(), 3)) {
            return 1;
        }
        
        // Verify first header
        if (!expect_eq_str("first header name", 
                          std::string(req.headers[0].name), 
                          "Content-Type")) {
            return 1;
        }
        
        // Verify second header
        if (!expect_eq_str("second header value", 
                          std::string(req.headers[1].value), 
                          "42")) {
            return 1;
        }
    }

    {
        std::cout << "Testing HttpRequest GET request..." << std::endl;
        HttpRequest req;
        req.method = "GET";
        req.path = "/index.html";
        req.version = 11;
        req.headers.push_back({"Host", "localhost:8080"});
        
        if (!expect_eq_str("GET method", std::string(req.method), "GET")) {
            return 1;
        }
        if (!expect_eq_str("GET path", std::string(req.path), "/index.html")) {
            return 1;
        }
        if (!expect_eq_int("GET version", req.version, 11)) {
            return 1;
        }
    }

    // ================== HttpResponse Tests ==================
    {
        std::cout << "Testing HttpResponse 200 OK..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        resp.set_body("Hello, World!");
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "HTTP/1.1 200 OK")) {
            return fail("missing status line");
        }
        if (!contains_substring(output, "Hello, World!")) {
            return fail("missing body");
        }
        if (!contains_substring(output, "Content-Length: 13")) {
            return fail("missing or incorrect content length");
        }
    }

    {
        std::cout << "Testing HttpResponse with headers..." << std::endl;
        HttpResponse resp(true);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        resp.set_content_type("text/html");
        resp.add_header("Cache-Control", "max-age=3600");
        resp.set_body("<html></html>");
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "Content-Type: text/html")) {
            return fail("missing Content-Type header");
        }
        if (!contains_substring(output, "Cache-Control: max-age=3600")) {
            return fail("missing custom header");
        }
        if (!contains_substring(output, "Connection: close")) {
            return fail("missing close connection header");
        }
    }

    {
        std::cout << "Testing HttpResponse 404 Not Found..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k404NotFound);
        resp.set_status_message("Not Found");
        resp.set_content_type("text/plain");
        resp.set_body("404 Not Found");
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "HTTP/1.1 404 Not Found")) {
            return fail("missing 404 status line");
        }
        if (!contains_substring(output, "404 Not Found")) {
            return fail("missing 404 body");
        }
    }

    {
        std::cout << "Testing HttpResponse 400 Bad Request..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k400BadRequest);
        resp.set_status_message("Bad Request");
        resp.set_body("Invalid request");
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "HTTP/1.1 400 Bad Request")) {
            return fail("missing 400 status line");
        }
    }

    {
        std::cout << "Testing HttpResponse 301 Moved Permanently..." << std::endl;
        HttpResponse resp(true);
        resp.set_status_code(HttpResponse::k301MovedPermanently);
        resp.set_status_message("Moved Permanently");
        resp.add_header("Location", "https://example.com");
        resp.set_body("");
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "HTTP/1.1 301 Moved Permanently")) {
            return fail("missing 301 status line");
        }
        if (!contains_substring(output, "Location: https://example.com")) {
            return fail("missing Location header");
        }
    }

    {
        std::cout << "Testing HttpResponse connection flags..." << std::endl;
        HttpResponse resp1(true);
        resp1.set_status_code(HttpResponse::k200Ok);
        resp1.set_status_message("OK");
        resp1.set_body("");
        
        if (!expect_eq_bool("close connection", resp1.is_close_connection(), true)) {
            return 1;
        }
        
        std::string output1 = resp1.to_string();
        if (!contains_substring(output1, "Connection: close")) {
            return fail("close connection flag not in response");
        }
        
        HttpResponse resp2(false);
        resp2.set_status_code(HttpResponse::k200Ok);
        resp2.set_status_message("OK");
        resp2.set_body("");
        
        if (!expect_eq_bool("keep-alive connection", resp2.is_close_connection(), false)) {
            return 1;
        }
        
        std::string output2 = resp2.to_string();
        if (!contains_substring(output2, "Connection: Keep-Alive")) {
            return fail("keep-alive flag not in response");
        }
    }

    {
        std::cout << "Testing HttpResponse empty body..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        resp.set_body("");
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "Content-Length: 0")) {
            return fail("missing content-length for empty body");
        }
    }

    {
        std::cout << "Testing HttpResponse large body..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        
        std::string large_body(10000, 'x');
        resp.set_body(large_body);
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "Content-Length: 10000")) {
            return fail("incorrect content-length for large body");
        }
        if (!contains_substring(output, large_body)) {
            return fail("large body not in response");
        }
    }

    {
        std::cout << "Testing HttpResponse multiple headers..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        resp.add_header("X-Custom-1", "value1");
        resp.add_header("X-Custom-2", "value2");
        resp.add_header("X-Custom-3", "value3");
        resp.set_body("test");
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, "X-Custom-1: value1")) {
            return fail("missing first custom header");
        }
        if (!contains_substring(output, "X-Custom-2: value2")) {
            return fail("missing second custom header");
        }
        if (!contains_substring(output, "X-Custom-3: value3")) {
            return fail("missing third custom header");
        }
    }

    {
        std::cout << "Testing HttpResponse header format..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        resp.set_content_type("application/json");
        resp.set_body("{\"key\": \"value\"}");
        
        std::string output = resp.to_string();
        
        // Verify proper format with \r\n
        size_t header_end = output.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return fail("missing header/body separator");
        }
        
        // Body should start after \r\n\r\n
        std::string body = output.substr(header_end + 4);
        if (!expect_eq_str("body content", body, "{\"key\": \"value\"}")) {
            return 1;
        }
    }

    {
        std::cout << "Testing HttpResponse set_close_connection..." << std::endl;
        HttpResponse resp(false);
        
        if (!expect_eq_bool("initial close_connection", resp.is_close_connection(), false)) {
            return 1;
        }
        
        resp.set_close_connection(true);
        
        if (!expect_eq_bool("after set_close_connection", resp.is_close_connection(), true)) {
            return 1;
        }
    }

    {
        std::cout << "Testing HttpResponse with special characters in body..." << std::endl;
        HttpResponse resp(false);
        resp.set_status_code(HttpResponse::k200Ok);
        resp.set_status_message("OK");
        
        std::string body_with_special = "Hello\nWorld\t!\r\n";
        resp.set_body(body_with_special);
        
        std::string output = resp.to_string();
        
        if (!contains_substring(output, body_with_special)) {
            return fail("special characters lost in body");
        }
    }

    std::cout << "\n✓ All tests passed!" << std::endl;
    return 0;
}
