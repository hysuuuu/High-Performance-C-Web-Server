# High-Performance Asynchronous Web Server

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

## Introduction

A non-blocking HTTP/1.1 web server built from scratch using C++17 on Linux.

The current implementation uses a multi-reactor architecture: a main acceptor loop dispatches new connections to sub-reactor event loops using round-robin. It serves static files from the local www directory and uses picohttpparser for lightweight HTTP parsing.

**Current Status:** Active development. Multi-reactor event loops, thread pool processing, and static file server are working. Advanced routing and HTTP features are still in progress.

## Key Features (Current)

- Event-driven I/O with edge-triggered epoll and non-blocking sockets.
- Multi-reactor event loops with round-robin dispatch from the main acceptor.
- Connection management via Channel/Connection classes, cleaned up with RAII patterns.
- Thread pool for parsing and response generation on worker threads.
- HTTP/1.1 parsing via picohttpparser.
- Keep-Alive support plus idle timeout sweeping (default 15 seconds).
- Static file serving with basic MIME type detection.

## System Architecture

The server uses a main acceptor loop plus a sub-reactor pool for I/O, and a worker pool for request processing:

```text
+------------------+      +------------------+
|  Acceptor        | ---> |  Main Loop       |
|  (listen socket) |      |  (epoll wait)    |
+--------+---------+      +--------+---------+
         |                         |
         | round-robin             | dispatch new connections
         v                         v
+------------------+      +------------------+
| Sub Reactor Pool | <--> |  Connection      |
| (N Event Loops)  |      |  Buffer + Channel|
+--------+---------+      +------------------+
         |
         | dispatch request
         v
+------------------+
| Worker Threadpool|
+------------------+
```

## HTTP Parsing

The network and buffering layers are custom, while HTTP parsing uses picohttpparser for low overhead, zero-copy style request parsing.

## Build and Run

```bash
mkdir -p build
cd build
cmake ..
make -j
./server
```

Server listens on port 8888 by default. Access http://localhost:8888/ to load www/index.html.

## Tests

```bash
cd build
ctest
```

## Docker

```bash
docker build -t hpcpp-server .
docker run --rm -p 8888:8888 hpcpp-server
```
