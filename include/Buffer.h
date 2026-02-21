#pragma once

#include <iostream> 
#include <sys/types.h>
#include <vector>
#include <string>
#include <algorithm>

/*
/--- (Prepend) ---/--- (Readable) ---/--- (Writable) ---/
|                 |                  |                  |
0            readerIndex         writerIndex           size()
*/

class Buffer {
private:
    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;

    char* begin() { return buffer_.data(); }
    const char* begin() const { return buffer_.data(); }

    void make_space(size_t len) {
        if (get_writable_bytes() + get_prependable_bytes() < len + k_cheap_prepend) {
            // Not enough space, resize buffer
            buffer_.resize(writer_index_ + len);
        } else {
            // The total space is enough, but it's fragmented.
            // Move readable data to the front to reclaim space
            size_t readable = get_readable_bytes();
            std::copy(begin() + reader_index_, begin() + writer_index_, begin() + k_cheap_prepend);
            reader_index_ = k_cheap_prepend;
            writer_index_ = reader_index_ + readable;
        }
    }

public:
    static const size_t k_cheap_prepend = 8;
    static const size_t k_initial_size = 1024;
    explicit Buffer(size_t initial_size = k_initial_size) : buffer_(k_cheap_prepend + initial_size), reader_index_(k_cheap_prepend), writer_index_(k_cheap_prepend) {}
    ~Buffer() = default;
    
    // Get readable data pointer 
    const char* peek() const {
        return begin() + reader_index_;
    }

    void retrieve(size_t len) {
        if (len < get_readable_bytes()) {
            reader_index_ += len; 
        } else {
            retrieve_all(); 
        }
    }

    // Reset buffer
    void retrieve_all() {
        reader_index_ = k_cheap_prepend;
        writer_index_ = k_cheap_prepend;
    }

    std::string retrieve_all_as_string() {
        std::string str(peek(), get_readable_bytes());
        retrieve_all();
        return str;
    }

    // Ensure there is enough space for write 
    void ensure_writable_bytes(size_t len) {
        if (get_writable_bytes() < len) {
            make_space(len);
        }
    }

    void append(const char* data, size_t len) {
        ensure_writable_bytes(len);
        std::copy(data, data + len, begin_write());
        writer_index_ += len;
    }

    void append(const std::string& str) {
        append(str.data(), str.length());
    }

    char* begin_write() { return begin() + writer_index_; }
    const char* begin_write() const { return begin() + writer_index_; }

    ssize_t read_fd(int fd, int* save_errno);

    // Getter
    size_t get_readable_bytes() const {
        return writer_index_ - reader_index_;
    }

    size_t get_writable_bytes() const {
        return buffer_.size() - writer_index_;
    }

    size_t get_prependable_bytes() const {
        return reader_index_;
    }

};