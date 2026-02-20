#pragma once

#include <string>
#include <unordered_map>

class HttpResponse {
public:
    // Standard HTTP Status Codes
    enum HttpStatusCode {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close) : status_code_(kUnknown), close_connection_(close) {}

    // Setters for Response Metadata
    void set_status_code(HttpStatusCode code) { status_code_ = code; }
    void set_status_message(const std::string& message) { status_message_ = message; }
    void set_close_connection(bool on) { close_connection_ = on; }
    bool is_close_connection() const { return close_connection_; }

    // Header management
    void set_content_type(const std::string& content_type) { add_header("Content-Type", content_type); }
    void add_header(const std::string& key, const std::string& value) { headers_[key] = value; }
    
    // Body management
    void set_body(const std::string& body) { body_ = body; }

    /**
     * Converts the internal state into a raw HTTP response string.
     * Follows the format: [Status Line]\r\n[Headers]\r\n\r\n[Body]
     */
    std::string to_string() const {
        std::string output;
        
        // tatus Line
        output += "HTTP/1.1 " + std::to_string(status_code_) + " " + status_message_ + "\r\n";

        // Standard Headers
        if (close_connection_) {
            output += "Connection: close\r\n";
        } else {
            output += "Connection: Keep-Alive\r\n";
        }

        output += "Content-Length: " + std::to_string(body_.size()) + "\r\n";

        // Custom Headers
        for (const auto& header : headers_) {
            output += header.first + ": " + header.second + "\r\n";
        }

        // Critical empty line separating headers and body
        output += "\r\n";

        output += body_;

        return output;
    }

private:
    std::unordered_map<std::string, std::string> headers_;
    HttpStatusCode status_code_;
    std::string status_message_;
    bool close_connection_;
    std::string body_;
};