#include "Buffer.h"

#include <sys/uio.h> 
#include <unistd.h> 
#include <errno.h>


/** 
 * Read data from fd using readv
 * If the data is smaller than the remaining Buffer space, store it directly
 * If the data is larger than the remaining Buffer space, 
 * store the exceeding part into extrabuf,
 * then append to Buffer later.
*/
ssize_t Buffer::read_fd(int fd, int* save_errno) {
    char extrabuf[65536];
    struct iovec vec[2];

    const size_t writable = get_writable_bytes();
    
    // Writable space for Buffer
    vec[0].iov_base = begin() + writer_index_;
    vec[0].iov_len = writable;  

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) {
        *save_errno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        // Enough space, write into Buffer
        writer_index_ += n;
    } else {
        writer_index_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}